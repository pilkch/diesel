// Standard headers
#include <cassert>
#include <iostream>
#include <limits>

// Gtk headers
#include <gdk/gdkkeysyms-compat.h>

// Cairomm headers
#include <cairomm/context.h>

// libopenglmm headers
#include <libopenglmm/cContext.h>
#include <libopenglmm/cGeometry.h>
#include <libopenglmm/cShader.h>
#include <libopenglmm/cTexture.h>
#include <libopenglmm/cVertexBufferObject.h>

// Spitfire headers
#include <spitfire/storage/filesystem.h>

// Diesel headers
#include "gtkmmopenglview.h"
#include "gtkmmphotobrowser.h"

namespace diesel
{
  class cGtkmmOpenGLViewEvent
  {
  public:
    virtual ~cGtkmmOpenGLViewEvent() {}

    virtual void EventFunction(cGtkmmOpenGLView& view) = 0;
  };


  class cGtkmmOpenGLViewFolderFoundEvent : public cGtkmmOpenGLViewEvent
  {
  public:
    cGtkmmOpenGLViewFolderFoundEvent(const string_t& sFolderName);

    virtual void EventFunction(cGtkmmOpenGLView& view) override;

    string_t sFolderName;
  };

  cGtkmmOpenGLViewFolderFoundEvent::cGtkmmOpenGLViewFolderFoundEvent(const string_t& _sFolderName) :
    sFolderName(_sFolderName)
  {
  }

  void cGtkmmOpenGLViewFolderFoundEvent::EventFunction(cGtkmmOpenGLView& view)
  {
    view.OnFolderFound(sFolderName);
  }


  class cGtkmmOpenGLViewFileFoundEvent : public cGtkmmOpenGLViewEvent
  {
  public:
    cGtkmmOpenGLViewFileFoundEvent(const string_t& sFileNameNoExtension);

    virtual void EventFunction(cGtkmmOpenGLView& view) override;

    string_t sFileNameNoExtension;
  };

  cGtkmmOpenGLViewFileFoundEvent::cGtkmmOpenGLViewFileFoundEvent(const string_t& _sFileNameNoExtension) :
    sFileNameNoExtension(_sFileNameNoExtension)
  {
  }

  void cGtkmmOpenGLViewFileFoundEvent::EventFunction(cGtkmmOpenGLView& view)
  {
    view.OnFileFound(sFileNameNoExtension);
  }


  class cGtkmmOpenGLViewImageLoadedEvent : public cGtkmmOpenGLViewEvent
  {
  public:
    cGtkmmOpenGLViewImageLoadedEvent(const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, voodoo::cImage* pImage);
    ~cGtkmmOpenGLViewImageLoadedEvent();

    virtual void EventFunction(cGtkmmOpenGLView& view) override;

    string_t sFileNameNoExtension;
    IMAGE_SIZE imageSize;
    voodoo::cImage* pImage;
  };

  cGtkmmOpenGLViewImageLoadedEvent::cGtkmmOpenGLViewImageLoadedEvent(const string_t& _sFileNameNoExtension, IMAGE_SIZE _imageSize, voodoo::cImage* _pImage) :
    sFileNameNoExtension(_sFileNameNoExtension),
    imageSize(_imageSize),
    pImage(_pImage)
  {
  }

  cGtkmmOpenGLViewImageLoadedEvent::~cGtkmmOpenGLViewImageLoadedEvent()
  {
    spitfire::SAFE_DELETE(pImage);
  }

  void cGtkmmOpenGLViewImageLoadedEvent::EventFunction(cGtkmmOpenGLView& view)
  {
    view.OnImageLoaded(sFileNameNoExtension, imageSize, pImage);
  }


  class cGtkmmOpenGLViewImageErrorEvent : public cGtkmmOpenGLViewEvent
  {
  public:
    explicit cGtkmmOpenGLViewImageErrorEvent(const string_t& sFileNameNoExtension);

    virtual void EventFunction(cGtkmmOpenGLView& view) override;

    string_t sFileNameNoExtension;
  };

  cGtkmmOpenGLViewImageErrorEvent::cGtkmmOpenGLViewImageErrorEvent(const string_t& _sFileNameNoExtension) :
    sFileNameNoExtension(_sFileNameNoExtension)
  {
  }

  void cGtkmmOpenGLViewImageErrorEvent::EventFunction(cGtkmmOpenGLView& view)
  {
    view.OnImageError(sFileNameNoExtension);
  }


  const float fThumbNailWidth = 100.0f;
  const float fThumbNailHeight = 100.0f;
  const float fThumbNailSpacing = 20.0f;

  // ** cPhotoEntry

  cPhotoEntry::cPhotoEntry() :
    state(STATE::LOADING),
    bLoadingFull(false),
    pTexturePhotoThumbnail(nullptr),
    pTexturePhotoFull(nullptr),
    pStaticVertexBufferObjectPhotoThumbnail(nullptr),
    pStaticVertexBufferObjectPhotoFull(nullptr),
    bIsSelected(false)
  {
  }

  // ** cGtkmmOpenGLView

  cGtkmmOpenGLView::cGtkmmOpenGLView(cGtkmmPhotoBrowser& _parent) :
    parent(_parent),
    imageLoadThread(*this),
    bIsWireframe(false),
    pageHeight(500),
    requiredHeight(12000),
    columns(10),
    fScale(1.0f),
    fScrollPosition(0.0f),
    pContext(nullptr),
    pTextureMissing(nullptr),
    pTextureFolder(nullptr),
    pTextureLoading(nullptr),
    pTextureLoadingError(nullptr),
    pShaderSelectionRectangle(nullptr),
    pShaderPhoto(nullptr),
    pShaderIcon(nullptr),
    pStaticVertexBufferObjectSelectionRectangle(nullptr),
    pStaticVertexBufferObjectIcon(nullptr),
    pFont(nullptr),
    bIsConfigureCalled(false),
    colourSelected(1.0f, 1.0f, 1.0f),
    bIsModeSinglePhoto(false),
    currentSinglePhoto(0),
    soAction("cGtkmmOpenGLView::soAction"),
    eventQueue(soAction)
  {
    // Set our resolution
    resolution.width = 500;
    resolution.height = 500;
    resolution.pixelFormat = opengl::PIXELFORMAT::R8G8B8A8;

    set_size_request(resolution.width, resolution.height);

    // Allow this control to grab the keyboard focus
    set_can_focus(true);


    // Allow this widget to handle button press and pointer events
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK);

    UpdateColumnsPageHeightAndRequiredHeight();

    imageLoadThread.Start();

    notifyMainThread.Create(*this, &cGtkmmOpenGLView::OnNotify);
  }

  cGtkmmOpenGLView::~cGtkmmOpenGLView()
  {
    Destroy();
  }

  void cGtkmmOpenGLView::Destroy()
  {
    imageLoadThread.StopNow();

    // Destroy any further events
    while (true) {
      cGtkmmOpenGLViewEvent* pEvent = eventQueue.RemoveItemFromFront();
      if (pEvent == nullptr) break;
      else spitfire::SAFE_DELETE(pEvent);
    }

    DestroyResources();

    DestroyOpenGL();
  }

  const GtkWidget* cGtkmmOpenGLView::GetWidget() const
  {
    return Gtk::Widget::gobj();
  }

  GtkWidget* cGtkmmOpenGLView::GetWidget()
  {
    return Gtk::Widget::gobj();
  }

  void cGtkmmOpenGLView::SetSelectionColour(const spitfire::math::cColour& colour)
  {
    colourSelected = colour;
  }

  string_t cGtkmmOpenGLView::GetFolder() const
  {
    return sFolderPath;
  }

  void cGtkmmOpenGLView::SetFolder(const string_t& _sFolderPath)
  {
    if (sFolderPath != _sFolderPath) {
      // Clear the request queue as soon as possible so that we don't waste time loading images from the old folder
      imageLoadThread.StopLoading();

      sFolderPath = _sFolderPath;

      if (bIsConfigureCalled) {
        // Reload our photos
        DestroyPhotos();
        CreatePhotos();
      }
    }
  }

  void cGtkmmOpenGLView::SetCacheMaximumSizeGB(size_t nCacheMaximumSizeGB)
  {
    imageLoadThread.SetMaximumCacheSizeGB(nCacheMaximumSizeGB);
  }

  void cGtkmmOpenGLView::StopLoading()
  {
    imageLoadThread.StopLoading();

    std::vector<cPhotoEntry*>::iterator iter = photos.begin();
    const std::vector<cPhotoEntry*>::iterator iterEnd = photos.end();
    while (iter != iterEnd) {
      if ((*iter)->state == cPhotoEntry::STATE::LOADING) (*iter)->state = cPhotoEntry::STATE::LOADING_ERROR;

      iter++;
    }

    parent.OnOpenGLViewLoadedFileOrFolder();
  }

  size_t cGtkmmOpenGLView::GetPhotoCount() const
  {
    return photos.size();
  }

  size_t cGtkmmOpenGLView::GetLoadedPhotoCount() const
  {
    size_t i = 0;

    std::vector<cPhotoEntry*>::const_iterator iter = photos.begin();
    const std::vector<cPhotoEntry*>::const_iterator iterEnd = photos.end();
    while (iter != iterEnd) {
      if ((*iter)->state != cPhotoEntry::STATE::LOADING) i++;

      iter++;
    }

    return i;
  }

  size_t cGtkmmOpenGLView::GetSelectedPhotoCount() const
  {
    size_t i = 0;

    std::vector<cPhotoEntry*>::const_iterator iter = photos.begin();
    const std::vector<cPhotoEntry*>::const_iterator iterEnd = photos.end();
    while (iter != iterEnd) {
      if ((*iter)->bIsSelected) i++;

      iter++;
    }

    return i;
  }

  size_t cGtkmmOpenGLView::GetPageHeight() const
  {
    return pageHeight;
  }

  size_t cGtkmmOpenGLView::GetRequiredHeight() const
  {
    return requiredHeight;
  }

  float cGtkmmOpenGLView::GetScale() const
  {
    return fScale;
  }

  void cGtkmmOpenGLView::SetScale(float _fScale)
  {
    fScale = _fScale;

    UpdateColumnsPageHeightAndRequiredHeight();
  }

  void cGtkmmOpenGLView::UpdateColumnsPageHeightAndRequiredHeight()
  {
    const float fWidth = (resolution.width - fThumbNailSpacing) / fScale;

    columns = max<size_t>(1, fWidth / (fThumbNailWidth + fThumbNailSpacing));

    const size_t y = max<size_t>(1, photos.size() / columns);
    const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));

    requiredHeight = fY;

    pageHeight = resolution.height / fScale;
  }

  bool cGtkmmOpenGLView::GetPhotoAtPoint(size_t& index, const spitfire::math::cVec2& _point) const
  {
    const spitfire::math::cVec2 point = spitfire::math::cVec2(0.0f, fScrollPosition) + _point / fScale;

    const size_t nPhotos = photos.size();
    for (size_t i = 0; i < nPhotos; i++) {
      const size_t x = i % columns;
      const size_t y = i / columns;
      const float fX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
      const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));

      if (
        (point.x >= fX) && (point.x <= fX + fThumbNailWidth) &&
        (point.y >= fY) && (point.y <= fY + fThumbNailHeight)
      ) {
        LOG<<"cGtkmmOpenGLView::GetPhotoAtPoint Clicked in "<<i<<std::endl;
        index = i;
        return true;
      }
    }

    LOG<<"cGtkmmOpenGLView::GetPhotoAtPoint Clicked in blank space"<<std::endl;
    return false;
  }

  void cGtkmmOpenGLView::CreateVertexBufferObjectSelectionRectangle(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight)
  {
    ASSERT(pStaticVertexBufferObject != nullptr);

    opengl::cGeometryDataPtr pGeometryDataPtr = opengl::CreateGeometryData();

    const spitfire::math::cVec2 vMin(0.0f, 0.0f);
    const spitfire::math::cVec2 vMax(fWidth, fHeight);

    opengl::cGeometryBuilder_v2 builder(*pGeometryDataPtr);

    // Front facing rectangle
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMin.y));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMax.y));
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMax.y));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMin.y));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMax.y));
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMin.y));

    pStaticVertexBufferObject->SetData(pGeometryDataPtr);

    pStaticVertexBufferObject->Compile2D(system);
  }

  void cGtkmmOpenGLView::CreateVertexBufferObjectSquare(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight)
  {
    ASSERT(pStaticVertexBufferObject != nullptr);

    opengl::cGeometryDataPtr pGeometryDataPtr = opengl::CreateGeometryData();

    const spitfire::math::cVec2 vMin(0.0f, 0.0f);
    const spitfire::math::cVec2 vMax(fWidth, fHeight);

    opengl::cGeometryBuilder_v2_t2 builder(*pGeometryDataPtr);

    // Front facing rectangle
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMin.y), spitfire::math::cVec2(1.0f, 0.0f));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMax.y), spitfire::math::cVec2(0.0f, 1.0f));
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMax.y), spitfire::math::cVec2(1.0f, 1.0f));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMin.y), spitfire::math::cVec2(0.0f, 0.0f));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMax.y), spitfire::math::cVec2(0.0f, 1.0f));
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMin.y), spitfire::math::cVec2(1.0f, 0.0f));

    pStaticVertexBufferObject->SetData(pGeometryDataPtr);

    pStaticVertexBufferObject->Compile2D(system);
  }

  void cGtkmmOpenGLView::CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fX, float fY, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight)
  {
    ASSERT(pStaticVertexBufferObject != nullptr);

    opengl::cGeometryDataPtr pGeometryDataPtr = opengl::CreateGeometryData();

    const float fTextureWidth = textureWidth;
    const float fTextureHeight = textureHeight;

    const spitfire::math::cVec2 vMin(fX, fY);
    const spitfire::math::cVec2 vMax(vMin.x + fWidth, vMin.y + fHeight);

    opengl::cGeometryBuilder_v2_t2 builder(*pGeometryDataPtr);

    // Front facing rectangle
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMin.y), spitfire::math::cVec2(fTextureWidth, 0.0f));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMax.y), spitfire::math::cVec2(0.0f, fTextureHeight));
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMax.y), spitfire::math::cVec2(fTextureWidth, fTextureHeight));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMin.y), spitfire::math::cVec2(0.0f, 0.0f));
    builder.PushBack(spitfire::math::cVec2(vMin.x, vMax.y), spitfire::math::cVec2(0.0f, fTextureHeight));
    builder.PushBack(spitfire::math::cVec2(vMax.x, vMin.y), spitfire::math::cVec2(fTextureWidth, 0.0f));

    pStaticVertexBufferObject->SetData(pGeometryDataPtr);

    pStaticVertexBufferObject->Compile2D(system);
  }

  void cGtkmmOpenGLView::CreateVertexBufferObjectIcon()
  {
    pStaticVertexBufferObjectIcon = pContext->CreateStaticVertexBufferObject();
    ASSERT(pStaticVertexBufferObjectIcon != nullptr);
    const float fWidthAndHeight = min(fThumbNailWidth, fThumbNailHeight);
    CreateVertexBufferObjectSquare(pStaticVertexBufferObjectIcon, fWidthAndHeight, fWidthAndHeight);
  }

  void cGtkmmOpenGLView::CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto, size_t textureWidth, size_t textureHeight)
  {
    ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
    const float fRatio = float(textureWidth) / float(textureHeight);
    float fWidth = fThumbNailWidth;
    float fHeight = fWidth * (1.0f / fRatio);
    if (fHeight > fThumbNailHeight) {
      fHeight = fThumbNailHeight;
      fWidth = fHeight * fRatio;
    }
    // Center the photo
    const float fX = 0.5f * (fThumbNailWidth - fWidth);
    const float fY = 0.5f * (fThumbNailHeight - fHeight);
    CreateVertexBufferObjectRect(pStaticVertexBufferObjectPhoto, fX, fY, fWidth, fHeight, textureWidth, textureHeight);
  }

  /*void cGtkmmOpenGLView::CreateVertexBufferObjectPhotos()
  {
    if (pTexture != nullptr) {

    }
  }*/

  void cGtkmmOpenGLView::CreateResources()
  {
    // Return if we have already created our resources
    if (pShaderPhoto != nullptr) {
      // Recreate the vertex buffer object
      if (pStaticVertexBufferObjectIcon != nullptr) {
        pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectIcon);
        pStaticVertexBufferObjectIcon = nullptr;
      }

      // Create our vertex buffer object
      CreateVertexBufferObjectIcon();

      // Recreate the vertex buffer object
      //DestroyVertexBufferObjectPhotos();

      // Create our vertex buffer objects
      //CreateVertexBufferObjectPhotos();

      return;
    }

    DestroyResources();

    CreatePhotos();

    pStaticVertexBufferObjectSelectionRectangle = pContext->CreateStaticVertexBufferObject();
    ASSERT(pStaticVertexBufferObjectSelectionRectangle != nullptr);
    const float fWidthAndHeight = max(fThumbNailWidth, fThumbNailHeight);
    CreateVertexBufferObjectSelectionRectangle(pStaticVertexBufferObjectSelectionRectangle, fWidthAndHeight, fWidthAndHeight);

    // Create our vertex buffer object
    CreateVertexBufferObjectIcon();

    // Create our vertex buffer objects
    //CreateVertexBufferObjectPhotos();

    // Create our shaders
    pShaderSelectionRectangle = pContext->CreateShader(TEXT("data/shaders/colour.vert"), TEXT("data/shaders/colour.frag"));
    ASSERT(pShaderSelectionRectangle != nullptr);

    pShaderPhoto = pContext->CreateShader(TEXT("data/shaders/passthrough.vert"), TEXT("data/shaders/passthrough_recttexture.frag"));
    ASSERT(pShaderPhoto != nullptr);

    pShaderIcon = pContext->CreateShader(TEXT("data/shaders/passthrough.vert"), TEXT("data/shaders/passthrough_squaretexture_alphamask.frag"));
    ASSERT(pShaderIcon != nullptr);

    // Create our font
    pFont = pContext->CreateFont(TEXT("data/fonts/pricedown.ttf"), 32, TEXT("data/shaders/font.vert"), TEXT("data/shaders/font.frag"));
    assert(pFont != nullptr);
    assert(pFont->IsValid());

    pTextureMissing = pContext->CreateTexture("data/textures/icon_question_mark.png");
    ASSERT(pTextureMissing != nullptr);
    pTextureFolder = pContext->CreateTexture("data/textures/icon_folder.png");
    ASSERT(pTextureFolder != nullptr);
    pTextureLoading = pContext->CreateTexture("data/textures/icon_stopwatch.png");
    ASSERT(pTextureLoading != nullptr);
    pTextureLoadingError = pContext->CreateTexture("data/textures/icon_loading_error.png");
    ASSERT(pTextureLoadingError != nullptr);
  }

  void cGtkmmOpenGLView::DestroyResources()
  {
    if (pTextureLoadingError != nullptr) {
      pContext->DestroyTexture(pTextureLoadingError);
      pTextureLoadingError = nullptr;
    }
    if (pTextureLoading != nullptr) {
      pContext->DestroyTexture(pTextureLoading);
      pTextureLoading = nullptr;
    }
    if (pTextureFolder != nullptr) {
      pContext->DestroyTexture(pTextureFolder);
      pTextureFolder = nullptr;
    }
    if (pTextureMissing != nullptr) {
      pContext->DestroyTexture(pTextureMissing);
      pTextureMissing = nullptr;
    }

    if (pFont != nullptr) {
      pContext->DestroyFont(pFont);
      pFont = nullptr;
    }

    if (pStaticVertexBufferObjectSelectionRectangle != nullptr) {
      pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectSelectionRectangle);
      pStaticVertexBufferObjectSelectionRectangle = nullptr;
    }

    if (pStaticVertexBufferObjectIcon != nullptr) {
      pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectIcon);
      pStaticVertexBufferObjectIcon = nullptr;
    }

    /*DestroyStaticVertexBufferObjectPhotos();

    DestroyStaticVertexBufferObjectPhotos()
    {
      if (pStaticVertexBufferObjectPhoto != nullptr) {
        pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectPhoto);
        pStaticVertexBufferObjectPhoto = nullptr;
      }
    }*/

    if (pShaderIcon != nullptr) {
      pContext->DestroyShader(pShaderIcon);
      pShaderIcon = nullptr;
    }

    if (pShaderPhoto != nullptr) {
      pContext->DestroyShader(pShaderPhoto);
      pShaderPhoto = nullptr;
    }

    if (pShaderSelectionRectangle != nullptr) {
      pContext->DestroyShader(pShaderSelectionRectangle);
      pShaderSelectionRectangle = nullptr;
    }

    DestroyPhotos();
  }

  void cGtkmmOpenGLView::CreatePhotos()
  {
    // If we don't have a folder to show then we can just return
    if (sFolderPath.empty()) return;

    // Reset our files loaded and total count
    parent.OnOpenGLViewLoadedFilesClear();

    // Tell our image loading thread to start loading the folder
    imageLoadThread.LoadFolderThumbnails(sFolderPath);
  }

  void cGtkmmOpenGLView::DestroyPhotos()
  {
    const size_t n = photos.size();
    for (size_t i = 0; i < n; i++) {
      if (photos[i]->pTexturePhotoThumbnail != nullptr) pContext->DestroyTexture(photos[i]->pTexturePhotoThumbnail);
      if (photos[i]->pTexturePhotoFull != nullptr) pContext->DestroyTexture(photos[i]->pTexturePhotoFull);

      if (photos[i]->pStaticVertexBufferObjectPhotoThumbnail != nullptr) pContext->DestroyStaticVertexBufferObject(photos[i]->pStaticVertexBufferObjectPhotoThumbnail);
      if (photos[i]->pStaticVertexBufferObjectPhotoFull != nullptr) pContext->DestroyStaticVertexBufferObject(photos[i]->pStaticVertexBufferObjectPhotoFull);

      spitfire::SAFE_DELETE(photos[i]);
    }

    photos.clear();
  }

  void cGtkmmOpenGLView::Init(int argc, char* argv[])
  {
    gtk_widget_set_events(GetWidget(), GDK_EXPOSURE_MASK);

    InitOpenGL(argc, argv);

    grab_focus();
  }

  void cGtkmmOpenGLView::InitOpenGL(int argc, char* argv[])
  {
    gtk_gl_init(&argc, &argv);

    // Prepare OpenGL
    GdkGLConfigMode flags = GdkGLConfigMode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);
    #ifndef GDK_MULTIHEAD_SAFE
    GdkGLConfig* pConfig = gdk_gl_config_new_by_mode(flags);
    if (pConfig == nullptr) {
      LOG<<"cGtkmmOpenGLView::InitOpenGL gdk_gl_config_new_by_mode FAILED for double buffered rendering"<<std::endl;
      LOG<<"cGtkmmOpenGLView::InitOpenGL Trying single buffered rendering"<<std::endl;

      // Try single-buffered visual
      flags = GdkGLConfigMode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH);
      pConfig = gdk_gl_config_new_by_mode(flags);
      if (pConfig == nullptr) LOG<<"cGtkmmOpenGLView::InitOpenGL gdk_gl_config_new_by_mode FAILED for single buffered rendering"<<std::endl;
    }
    #else
    GdkGLConfig* pConfig = gdk_gl_config_new_by_mode_for_screen(GdkScreen* screen, flags);
    #endif
    ASSERT(pConfig != nullptr);

    GtkWidget* pWidget = GetWidget();

    if (!gtk_widget_set_gl_capability(pWidget, pConfig, NULL, TRUE, GDK_GL_RGBA_TYPE)) g_assert_not_reached();

    // Handle the configure event
    g_signal_connect(pWidget, "configure-event", G_CALLBACK(configure_cb), (void*)this);

    // Create our redraw timer
    const gdouble TIMEOUT_PERIOD = 1000 / 60;
    g_timeout_add(TIMEOUT_PERIOD, idle_cb, pWidget);

    // Init libopenglmm
    pContext = system.CreateSharedContextForWidget(resolution);
  }

  gboolean cGtkmmOpenGLView::configure_cb(GtkWidget* pWidget, GdkEventConfigure* event, gpointer pUserData)
  {
    gtk_widget_begin_gl(pWidget);

    // Set the viewport
    GtkAllocation alloc;
    gtk_widget_get_allocation(pWidget, &alloc);

    cGtkmmOpenGLView* pThis = (cGtkmmOpenGLView*)pUserData;
    ASSERT(pThis != nullptr);
    pThis->bIsConfigureCalled = true;
    pThis->ResizeWidget(alloc.width, alloc.height);
    pThis->CreateResources();

    const bool bSwap = true;
    gtk_widget_end_gl(pWidget, bSwap);

    return TRUE;
  }

  void cGtkmmOpenGLView::DestroyOpenGL()
  {
    // Destroy our context
    if (pContext != nullptr) {
      system.DestroyContext(pContext);
      pContext = nullptr;
    }
  }

  void cGtkmmOpenGLView::ResizeWidget(size_t width, size_t height)
  {
    resolution.width = width;
    resolution.height = height;
    pContext->ResizeWindow(resolution);

    UpdateColumnsPageHeightAndRequiredHeight();

    // Notify the parent that our view has changed
    parent.OnOpenGLViewContentChanged();
  }

  void cGtkmmOpenGLView::RenderPhoto(size_t index, const spitfire::math::cMat4& matScale)
  {
    spitfire::math::cMat4 matModelView2D;

    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto = photos[index]->pStaticVertexBufferObjectPhotoThumbnail;

    opengl::cTexture* pTexture = photos[index]->pTexturePhotoThumbnail;

    if (bIsModeSinglePhoto && (photos[index]->pTexturePhotoFull != nullptr) && (photos[index]->pStaticVertexBufferObjectPhotoFull != nullptr)) {
      ASSERT(photos[index]->pTexturePhotoFull != nullptr);
      ASSERT(photos[index]->pStaticVertexBufferObjectPhotoFull != nullptr);
      pTexture = photos[index]->pTexturePhotoFull;
      pStaticVertexBufferObjectPhoto = photos[index]->pStaticVertexBufferObjectPhotoFull;
    }

    if ((pTexture != nullptr) && pTexture->IsValid() && (pStaticVertexBufferObjectPhoto != nullptr) && pStaticVertexBufferObjectPhoto->IsCompiled()) {

      pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);

      pContext->BindTexture(0, *pTexture);

      pContext->BindShader(*pShaderPhoto);

      if (!bIsModeSinglePhoto) {
        const size_t x = index % columns;
        const size_t y = index / columns;
        const float fX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
        const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));
        matModelView2D.SetTranslation(fX, fY - fScrollPosition, 0.0f);
      }

      pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

      pContext->DrawStaticVertexBufferObjectTriangles2D(*pStaticVertexBufferObjectPhoto);

      pContext->UnBindShader(*pShaderPhoto);

      pContext->UnBindTexture(0, *pTexture);

      pContext->UnBindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);
    } else {
      // Draw the icon
      pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectIcon);

      opengl::cTexture* pTexture = pTextureMissing;

      switch (photos[index]->state) {
        case cPhotoEntry::STATE::FOLDER: {
          pTexture = pTextureFolder;
          break;
        }
        case cPhotoEntry::STATE::LOADING: {
          pTexture = pTextureLoading;
          break;
        }
        case cPhotoEntry::STATE::LOADING_ERROR: {
          pTexture = pTextureLoadingError;
          break;
        }
      };

      pContext->BindTexture(0, *pTexture);

      pContext->BindShader(*pShaderIcon);

      const size_t x = index % columns;
      const size_t y = index / columns;
      const float fX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
      const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));
      matModelView2D.SetTranslation(fX, fY - fScrollPosition, 0.0f);

      pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

      pContext->DrawStaticVertexBufferObjectTriangles2D(*pStaticVertexBufferObjectIcon);

      pContext->UnBindShader(*pShaderIcon);

      pContext->UnBindTexture(0, *pTexture);

      pContext->UnBindStaticVertexBufferObject2D(*pStaticVertexBufferObjectIcon);
    }
  }

  void cGtkmmOpenGLView::DrawScene()
  {
    ASSERT(pContext != nullptr);
    ASSERT(pContext->IsValid());

    ASSERT(pStaticVertexBufferObjectSelectionRectangle != nullptr);
    ASSERT(pStaticVertexBufferObjectSelectionRectangle->IsCompiled());
    ASSERT(pStaticVertexBufferObjectIcon != nullptr);
    ASSERT(pStaticVertexBufferObjectIcon->IsCompiled());

    ASSERT(pShaderSelectionRectangle != nullptr);
    ASSERT(pShaderSelectionRectangle->IsCompiledProgram());
    ASSERT(pShaderPhoto != nullptr);
    ASSERT(pShaderPhoto->IsCompiledProgram());
    ASSERT(pShaderIcon != nullptr);
    ASSERT(pShaderIcon->IsCompiledProgram());

    pContext->SetClearColour(spitfire::math::cColour(0.0f, 0.0f, 0.0f, 1.0f));

    pContext->BeginRenderToScreen();

    if (bIsWireframe) pContext->EnableWireframe();

    // Render the photos
    if (!photos.empty()) {
      pContext->BeginRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO);

      spitfire::math::cMat4 matScale;

      if (bIsModeSinglePhoto) {
        // Single photo mode

        matScale.SetScale(10.0f * fScale, 10.0f * fScale, 1.0f);

        // Clamp the index to the possible photos
        ASSERT(!photos.empty());
        if (currentSinglePhoto >= photos.size()) currentSinglePhoto = photos.size() - 1;

        // Render the photo
        RenderPhoto(currentSinglePhoto, matScale);

        // Render the filename for the photo
        assert(pFont != nullptr);
        assert(pFont->IsValid());

        // The font is designed for use with contexts with a width of 1.0 so we need to scale it here
        const float fTextScale = 500.0f; // Fixed scale
        const spitfire::math::cVec2 scale(fTextScale, fTextScale);

        const spitfire::math::cColour colour(1.0f, 1.0f, 1.0f, 1.0f);

        opengl::cGeometryDataPtr pTextGeometryDataPtr = opengl::CreateGeometryData();

        opengl::cGeometryBuilder_v2_c4_t2 builderText(*pTextGeometryDataPtr);

        const size_t index = currentSinglePhoto;

        {
          const size_t x = index % columns;
          const size_t y = index / columns;
          const float fPhotoX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
          const float fPhotoY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));

          // Place the text below the photo
          const float fNameX = fPhotoX;
          const float fNameY = fPhotoY + fThumbNailHeight;

          // Create the text for this photo
          const float fRotationDegrees = 0.0f;
          pFont->PushBack(builderText, photos[index]->sFileNameNoExtension, colour, spitfire::math::cVec2(fNameX, fNameY), fRotationDegrees, scale);
        }


        if (pTextGeometryDataPtr->nVertexCount != 0) {
          // Compile the vertex buffer object
          opengl::cStaticVertexBufferObject* pVBOText = pContext->CreateStaticVertexBufferObject();

          pVBOText->SetData(pTextGeometryDataPtr);

          pVBOText->Compile2D(system);

          // Bind the vertex buffer object
          pContext->BindStaticVertexBufferObject2D(*pVBOText);

          // Set the position of the text
          spitfire::math::cMat4 matModelView2D;
          matModelView2D.SetTranslation(0.0f, -fScrollPosition, 0.0f);

          pContext->BindFont(*pFont);

          pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

          pContext->DrawStaticVertexBufferObjectTriangles2D(*pVBOText);

          pContext->UnBindFont(*pFont);

          // Unbind the vertex buffer object
          pContext->UnBindStaticVertexBufferObject2D(*pVBOText);

          pContext->DestroyStaticVertexBufferObject(pVBOText);
        }
      } else {
        // Photo browsing mode

        matScale.SetScale(fScale, fScale, 1.0f);

        spitfire::math::cMat4 matModelView2D;

        const size_t nPhotos = photos.size();
        for (size_t i = 0; i < nPhotos; i++) {
          // Draw the selection rectangle
          if (photos[i]->bIsSelected) {
            // Draw the selection rectangle
            pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectSelectionRectangle);

            pContext->BindShader(*pShaderSelectionRectangle);

            pContext->SetShaderConstant("colour", colourSelected);

            const size_t x = i % columns;
            const size_t y = i / columns;
            const float fX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
            const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));
            matModelView2D.SetTranslation(fX, fY - fScrollPosition, 0.0f);

            pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

            pContext->DrawStaticVertexBufferObjectTriangles2D(*pStaticVertexBufferObjectSelectionRectangle);

            pContext->UnBindShader(*pShaderSelectionRectangle);

            pContext->UnBindStaticVertexBufferObject2D(*pStaticVertexBufferObjectSelectionRectangle);
          }

          // Render the photo
          RenderPhoto(i, matScale);
        }

        // Render the filenames for the photos
        assert(pFont != nullptr);
        assert(pFont->IsValid());

        // The font is designed for use with contexts with a width of 1.0 so we need to scale it here
        const float fTextScale = 500.0f; // Fixed scale
        const spitfire::math::cVec2 scale(fTextScale, fTextScale);

        const spitfire::math::cColour colour(1.0f, 1.0f, 1.0f, 1.0f);

        opengl::cGeometryDataPtr pTextGeometryDataPtr = opengl::CreateGeometryData();

        opengl::cGeometryBuilder_v2_c4_t2 builderText(*pTextGeometryDataPtr);

        const size_t n = photos.size();
        for (size_t i = 0; i < n; i++) {
          const size_t x = i % columns;
          const size_t y = i / columns;
          const float fPhotoX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
          const float fPhotoY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));

          // Place the text below the photo
          const float fNameX = fPhotoX;
          const float fNameY = fPhotoY + fThumbNailHeight;

          // Create the text for this photo
          const float fRotationDegrees = 0.0f;
          pFont->PushBack(builderText, photos[i]->sFileNameNoExtension, colour, spitfire::math::cVec2(fNameX, fNameY), fRotationDegrees, scale);
        }


        if (pTextGeometryDataPtr->nVertexCount != 0) {
          // Compile the vertex buffer object
          opengl::cStaticVertexBufferObject* pVBOText = pContext->CreateStaticVertexBufferObject();

          pVBOText->SetData(pTextGeometryDataPtr);

          pVBOText->Compile2D(system);

          // Bind the vertex buffer object
          pContext->BindStaticVertexBufferObject2D(*pVBOText);

          // Set the position of the text
          spitfire::math::cMat4 matModelView2D;
          matModelView2D.SetTranslation(0.0f, -fScrollPosition, 0.0f);

          pContext->BindFont(*pFont);

          pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

          pContext->DrawStaticVertexBufferObjectTriangles2D(*pVBOText);

          pContext->UnBindFont(*pFont);

          // Unbind the vertex buffer object
          pContext->UnBindStaticVertexBufferObject2D(*pVBOText);

          pContext->DestroyStaticVertexBufferObject(pVBOText);
        }
      }

      pContext->EndRenderMode2D();
    }

    if (bIsWireframe) pContext->DisableWireframe();

    pContext->EndRenderToScreen();
  }

  bool cGtkmmOpenGLView::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
  {
    // Draw with OpenGL instead of Cairo
    GtkWidget* pWidget = GetWidget();

    gtk_widget_begin_gl(pWidget);

    DrawScene();

    const bool bSwap = true;
    gtk_widget_end_gl(pWidget, bSwap);

    return true;
  }

  gboolean cGtkmmOpenGLView::idle_cb(gpointer pUserData)
  {
    //LOG<<"cGtkmmOpenGLView::idle_cb"<<std::endl;

    // Update control data/params  in this function if needed
    GtkWidget* pWidget = GTK_WIDGET(pUserData);

    // Force a redraw
    GdkRectangle dummyRect = { 0, 0, 1, 1 };
    gdk_window_invalidate_rect(gdk_gl_window_get_window(gtk_widget_get_gl_window(pWidget)), &dummyRect, TRUE);

    return TRUE;
  }

  void cGtkmmOpenGLView::OnNotify()
  {
    LOG<<"cGtkmmOpenGLView::OnNotify"<<std::endl;
    ASSERT(spitfire::util::IsMainThread());
    cGtkmmOpenGLViewEvent* pEvent = eventQueue.RemoveItemFromFront();
    if (pEvent != nullptr) {
      pEvent->EventFunction(*this);
      spitfire::SAFE_DELETE(pEvent);
    }
  }
  void cGtkmmOpenGLView::OnFolderFound(const string_t& sFolderName)
  {
    LOG<<"cGtkmmOpenGLView::OnFolderFound \""<<sFolderName<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cGtkmmOpenGLViewFolderFoundEvent* pEvent = new cGtkmmOpenGLViewFolderFoundEvent(sFolderName);
      eventQueue.AddItemToBack(pEvent);
      notifyMainThread.Notify();
    } else {
      LOG<<"cGtkmmOpenGLView::OnFolderFound On main thread \""<<sFolderName<<"\""<<std::endl;
      cPhotoEntry* pEntry = new cPhotoEntry;
      pEntry->sFileNameNoExtension = sFolderName;
      pEntry->state = cPhotoEntry::STATE::FOLDER;
      photos.push_back(pEntry);

      parent.OnOpenGLViewLoadedFileOrFolder(); // A folder counts as a loaded file
    }
  }

  void cGtkmmOpenGLView::OnFileFound(const string_t& sFileNameNoExtension)
  {
    LOG<<"cGtkmmOpenGLView::OnFileFound \""<<sFileNameNoExtension<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cGtkmmOpenGLViewFileFoundEvent* pEvent = new cGtkmmOpenGLViewFileFoundEvent(sFileNameNoExtension);
      eventQueue.AddItemToBack(pEvent);
      notifyMainThread.Notify();
    } else {
      LOG<<"cGtkmmOpenGLView::OnFileFound On main thread \""<<sFileNameNoExtension<<"\""<<std::endl;
      cPhotoEntry* pEntry = new cPhotoEntry;
      pEntry->sFileNameNoExtension = sFileNameNoExtension;
      pEntry->state = cPhotoEntry::STATE::LOADING;
      photos.push_back(pEntry);

      parent.OnOpenGLViewFileFound();
    }
  }

  void cGtkmmOpenGLView::OnImageError(const string_t& sFileNameNoExtension)
  {
    LOG<<"cGtkmmOpenGLView::OnImageError \""<<sFileNameNoExtension<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cGtkmmOpenGLViewImageErrorEvent* pEvent = new cGtkmmOpenGLViewImageErrorEvent(sFileNameNoExtension);
      eventQueue.AddItemToBack(pEvent);
      notifyMainThread.Notify();
    } else {
      LOG<<"cGtkmmOpenGLView::OnImageError On main thread \""<<sFileNameNoExtension<<"\""<<std::endl;
      const size_t n = photos.size();
      for (size_t i = 0; i < n; i++) {
        if (photos[i]->sFileNameNoExtension == sFileNameNoExtension) {
          cPhotoEntry* pEntry = photos[i];
          pEntry->state = cPhotoEntry::STATE::LOADING_ERROR;
          break;
        }
      }

      parent.OnOpenGLViewLoadedFileOrFolder();
    }
  }

  void cGtkmmOpenGLView::OnImageLoaded(const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, voodoo::cImage* pImage)
  {
    LOG<<"cGtkmmOpenGLView::OnImageLoaded \""<<sFileNameNoExtension<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cGtkmmOpenGLViewImageLoadedEvent* pEvent = new cGtkmmOpenGLViewImageLoadedEvent(sFileNameNoExtension, imageSize, pImage);
      eventQueue.AddItemToBack(pEvent);
      notifyMainThread.Notify();
    } else {
      LOG<<"cGtkmmOpenGLView::OnImageLoaded On main thread \""<<sFileNameNoExtension<<"\""<<std::endl;
      const size_t n = photos.size();
      for (size_t i = 0; i < n; i++) {
        if (photos[i]->sFileNameNoExtension == sFileNameNoExtension) {
          cPhotoEntry* pEntry = photos[i];
          pEntry->state = cPhotoEntry::STATE::LOADED;

          if (imageSize == IMAGE_SIZE::THUMBNAIL) {
            ASSERT(pEntry->pTexturePhotoThumbnail == nullptr);
            ASSERT(pEntry->pStaticVertexBufferObjectPhotoThumbnail == nullptr);

            // Create the texture
            pEntry->pTexturePhotoThumbnail = pContext->CreateTextureFromImage(*pImage);
            ASSERT(pEntry->pTexturePhotoThumbnail != nullptr);

            // Create the static vertex buffer object
            pEntry->pStaticVertexBufferObjectPhotoThumbnail = pContext->CreateStaticVertexBufferObject();
            CreateVertexBufferObjectPhoto(pEntry->pStaticVertexBufferObjectPhotoThumbnail, pEntry->pTexturePhotoThumbnail->GetWidth(), pEntry->pTexturePhotoThumbnail->GetHeight());
            ASSERT(pEntry->pStaticVertexBufferObjectPhotoThumbnail != nullptr);
          } else {
            ASSERT(pEntry->pTexturePhotoFull == nullptr);
            ASSERT(pEntry->pStaticVertexBufferObjectPhotoFull == nullptr);

            // Create the texture
            pEntry->pTexturePhotoFull = pContext->CreateTextureFromImage(*pImage);
            ASSERT(pEntry->pTexturePhotoFull != nullptr);

            // Create the static vertex buffer object
            pEntry->pStaticVertexBufferObjectPhotoFull = pContext->CreateStaticVertexBufferObject();
            CreateVertexBufferObjectPhoto(pEntry->pStaticVertexBufferObjectPhotoFull, pEntry->pTexturePhotoFull->GetWidth(), pEntry->pTexturePhotoFull->GetHeight());
            ASSERT(pEntry->pStaticVertexBufferObjectPhotoFull != nullptr);
          }

          break;
        }
      }

      parent.OnOpenGLViewLoadedFileOrFolder();
    }
  }

  void cGtkmmOpenGLView::PreloadSinglePhoto(size_t index)
  {
    ASSERT(index < photos.size());

    cPhotoEntry* pPhoto = photos[index];

    if (pPhoto->state != cPhotoEntry::STATE::FOLDER) {
      // Tell our image loading thread to start loading the full sized version of this image
      if ((pPhoto->pTexturePhotoFull == nullptr) && !pPhoto->bLoadingFull) {
        pPhoto->bLoadingFull = true;
        imageLoadThread.LoadFileFullHighPriority(pPhoto->sFileNameNoExtension);
      }
    }
  }

  void cGtkmmOpenGLView::SetCurrentSinglePhoto(size_t index)
  {
    ASSERT(index < photos.size());

    currentSinglePhoto = index;

    // Load this photo
    PreloadSinglePhoto(currentSinglePhoto);
  }

  bool cGtkmmOpenGLView::OnKeyPressEvent(GdkEventKey* pEvent)
  {
    LOG<<"cGtkmmOpenGLView::OnKeyPressEvent"<<std::endl;

    // Handle single photo mode
    if (bIsModeSinglePhoto) {
      switch (pEvent->keyval) {
        case GDK_Left:
        case GDK_Up:
        case GDK_Page_Up: {
          if (currentSinglePhoto != 0) SetCurrentSinglePhoto(currentSinglePhoto - 1);
          return true;
        }
        case GDK_Right:
        case GDK_Down:
        case GDK_Page_Down:
        case GDK_space: {
          if (currentSinglePhoto + 1 < photos.size()) SetCurrentSinglePhoto(currentSinglePhoto + 1);
          return true;
        }
        case GDK_Home: {
          SetCurrentSinglePhoto(0);
          return true;
        }
        case GDK_End: {
          if (!photos.empty()) SetCurrentSinglePhoto(photos.size() - 1);
          return true;
        }
        case GDK_Escape: {
          bIsModeSinglePhoto = false;
          return true;
        }
      }
    }

    // Handle default keys
    switch (pEvent->keyval) {
      #ifdef BUILD_DEBUG
      case GDK_w: {
        bIsWireframe = !bIsWireframe;
        return true;
      }
      #endif
      /*
      case GDK_Return:
        entersig.emit("yo");
        cout << "YES!" << endl;
        return true;*/
      case GDK_Home: {
        parent.OnOpenGLViewScrollTop();
        return true;
      }
      case GDK_End: {
        parent.OnOpenGLViewScrollBottom();
        return true;
      }
      case GDK_Page_Up: {
        parent.OnOpenGLViewScrollPageUp();
        return true;
      }
      case GDK_Page_Down: {
        parent.OnOpenGLViewScrollPageDown();
        return true;
      }
      case GDK_0: {
        if ((pEvent->state & GDK_CONTROL_MASK) != 0) {
          // Reset the zoom
          SetScale(1.0f);
          parent.OnOpenGLViewContentChanged();
          return true;
        }

        break;
      }
    }

    return false;
  }

  bool cGtkmmOpenGLView::OnMouseDown(int button, int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseDown"<<std::endl;

    // Set the focus to this widget so that arrow keys work
    grab_focus();

    // Change the selection on left and right click
    if ((button == 1) || (button == 3)) {
      const size_t n = photos.size();

      size_t index = 0;
      if (GetPhotoAtPoint(index, spitfire::math::cVec2(x, y))) {
        LOG<<"cGtkmmOpenGLView::OnMouseDown item="<<index<<std::endl;
        ASSERT(index < photos.size());
        if (!bKeyControl && !bKeyShift) {
          // Select only the photo we clicked on
          for (size_t i = 0; i < n; i++) photos[i]->bIsSelected = (i == index);
        } else if (bKeyControl && !bKeyShift) {
          // Toggle the selection of the photo we clicked on
          photos[index]->bIsSelected = !photos[index]->bIsSelected;
        }
      } else if (!bKeyControl && !bKeyShift) {
        // Clear the selection
        for (size_t i = 0; i < n; i++) photos[i]->bIsSelected = false;
      }

      parent.OnOpenGLViewSelectionChanged();
    }

    return true;
  }

  bool cGtkmmOpenGLView::OnMouseRelease(int button, int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseRelease"<<std::endl;

    // Handle right click
    if (button == 3) parent.OnOpenGLViewRightClick();

    return true;
  }

  bool cGtkmmOpenGLView::OnMouseDoubleClick(int button, int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseDoubleClick"<<std::endl;

    // Enter single photo mode
    if (button == 1) {
      if (!bIsModeSinglePhoto) {
        size_t index = 0;
        if (GetPhotoAtPoint(index, spitfire::math::cVec2(x, y))) {
          ASSERT(index < photos.size());
          if (!bKeyControl && !bKeyShift) {
            // Enter single photo mode
            bIsModeSinglePhoto = true;
            SetCurrentSinglePhoto(index);
          }
        }
      } else {
        // Exit single photo mode
        bIsModeSinglePhoto = false;
      }
    }

    return true;
  }

  bool cGtkmmOpenGLView::OnMouseScrollUp(int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseScrollUp"<<std::endl;
    return false;
  }

  bool cGtkmmOpenGLView::OnMouseScrollDown(int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseScrollDown"<<std::endl;
    return false;
  }

  bool cGtkmmOpenGLView::OnMouseMove(int x, int y, bool bKeyControl, bool bKeyShift)
  {
    //LOG<<"cGtkmmOpenGLView::OnMouseMove"<<std::endl;
    return false;
  }

  void cGtkmmOpenGLView::OnScrollBarScrolled(float fValue)
  {
    LOG<<"cGtkmmOpenGLView::OnScrollBarScrolled"<<std::endl;
    fScrollPosition = fValue;
  }
}
