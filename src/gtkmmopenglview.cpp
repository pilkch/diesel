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


  class cGtkmmOpenGLViewImageLoadedEvent : public cGtkmmOpenGLViewEvent
  {
  public:
    cGtkmmOpenGLViewImageLoadedEvent(const string_t& sFilePath, IMAGE_SIZE imageSize, voodoo::cImage* pImage);
    ~cGtkmmOpenGLViewImageLoadedEvent();

    virtual void EventFunction(cGtkmmOpenGLView& view) override;

    string_t sFilePath;
    IMAGE_SIZE imageSize;
    voodoo::cImage* pImage;
  };

  cGtkmmOpenGLViewImageLoadedEvent::cGtkmmOpenGLViewImageLoadedEvent(const string_t& _sFilePath, IMAGE_SIZE _imageSize, voodoo::cImage* _pImage) :
    sFilePath(_sFilePath),
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
    view.OnImageLoaded(sFilePath, imageSize, pImage);
  }


  class cGtkmmOpenGLViewImageErrorEvent : public cGtkmmOpenGLViewEvent
  {
  public:
    cGtkmmOpenGLViewImageErrorEvent(const string_t& sFilePath, IMAGE_SIZE imageSize);

    virtual void EventFunction(cGtkmmOpenGLView& view) override;

    string_t sFilePath;
    IMAGE_SIZE imageSize;
  };

  cGtkmmOpenGLViewImageErrorEvent::cGtkmmOpenGLViewImageErrorEvent(const string_t& _sFilePath, IMAGE_SIZE _imageSize) :
    sFilePath(_sFilePath),
    imageSize(_imageSize)
  {
  }

  void cGtkmmOpenGLViewImageErrorEvent::EventFunction(cGtkmmOpenGLView& view)
  {
    view.OnImageError(sFilePath, imageSize);
  }


  const float fThumbNailWidth = 100.0f;
  const float fThumbNailHeight = 100.0f;
  const float fThumbNailSpacing = 20.0f;

  // ** cPhotoEntry

  cPhotoEntry::cPhotoEntry() :
    state(STATE::LOADING),
    pTexture(nullptr),
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
    pStaticVertexBufferObjectPhoto(nullptr),
    pStaticVertexBufferObjectIcon(nullptr),
    pFont(nullptr),
    colourSelected(1.0f, 1.0f, 1.0f),
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
      sFolderPath = _sFolderPath;

      // Reload our photos
      DestroyPhotos();
      CreatePhotos();
    }
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

  void cGtkmmOpenGLView::CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight)
  {
    ASSERT(pStaticVertexBufferObject != nullptr);

    opengl::cGeometryDataPtr pGeometryDataPtr = opengl::CreateGeometryData();

    const float fTextureWidth = textureWidth;
    const float fTextureHeight = textureHeight;

    const spitfire::math::cVec2 vMin(0.0f, 0.0f);
    const spitfire::math::cVec2 vMax(fWidth, fHeight);

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

  void cGtkmmOpenGLView::CreateVertexBufferObjectPhoto()
  {
    // TODO: We need to cache a few sizes, every time we encounter a newly sized photo we need to create a new vertex buffer object
    pStaticVertexBufferObjectPhoto = pContext->CreateStaticVertexBufferObject();
    ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
    const size_t textureWidth = 3040;
    const size_t textureHeight = 2024;
    const float fRatio = float(textureWidth) / float(textureHeight);
    const float fWidth = fThumbNailWidth;
    const float fHeight = fWidth * (1.0f / fRatio);
    CreateVertexBufferObjectRect(pStaticVertexBufferObjectPhoto, fWidth, fHeight, textureWidth, textureHeight);
  }

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
      if (pStaticVertexBufferObjectPhoto != nullptr) {
        pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectPhoto);
        pStaticVertexBufferObjectPhoto = nullptr;
      }

      // Create our vertex buffer object
      CreateVertexBufferObjectPhoto();

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

    // Create our vertex buffer object
    CreateVertexBufferObjectPhoto();

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

    if (pStaticVertexBufferObjectPhoto != nullptr) {
      pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectPhoto);
      pStaticVertexBufferObjectPhoto = nullptr;
    }

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

    // Create our textures
    for (spitfire::filesystem::cFolderIterator iter(sFolderPath); iter.IsValid(); iter.Next()) {
      if (iter.IsFile()) {
        cPhotoEntry* pEntry = new cPhotoEntry;
        pEntry->sFilePath = iter.GetFullPath();
        if (spitfire::filesystem::IsFolder(pEntry->sFilePath)) pEntry->state = cPhotoEntry::STATE::FOLDER;
        else {
          // Start loading the image in the background
          pEntry->state = cPhotoEntry::STATE::LOADING;
          imageLoadThread.LoadThumbnail(pEntry->sFilePath, IMAGE_SIZE::FULL);
        }

        photos.push_back(pEntry);
      }
    }
  }

  void cGtkmmOpenGLView::DestroyPhotos()
  {
    const size_t n = photos.size();
    for (size_t i = 0; i < n; i++) {
      opengl::cTexture* pTexture = photos[i]->pTexture;
      if (pTexture != nullptr) pContext->DestroyTexture(pTexture);

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

  void cGtkmmOpenGLView::DrawScene()
  {
    ASSERT(pContext != nullptr);
    ASSERT(pContext->IsValid());

    ASSERT(pStaticVertexBufferObjectSelectionRectangle != nullptr);
    ASSERT(pStaticVertexBufferObjectSelectionRectangle->IsCompiled());
    ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
    ASSERT(pStaticVertexBufferObjectPhoto->IsCompiled());
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
    {
      pContext->BeginRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO);

      spitfire::math::cMat4 matModelView2D;

      spitfire::math::cMat4 matScale;
      matScale.SetScale(fScale, fScale, 1.0f);

      // Draw the photos
      {
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

          // Draw the photo
          if (photos[i]->pTexture != nullptr) {
            pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);

            opengl::cTexture* pTexture = photos[i]->pTexture;

            pContext->BindTexture(0, *pTexture);

            pContext->BindShader(*pShaderPhoto);

            const size_t x = i % columns;
            const size_t y = i / columns;
            const float fX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
            const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));
            matModelView2D.SetTranslation(fX, fY - fScrollPosition, 0.0f);

            pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

            pContext->DrawStaticVertexBufferObjectTriangles2D(*pStaticVertexBufferObjectPhoto);

            pContext->UnBindShader(*pShaderPhoto);

            pContext->UnBindTexture(0, *pTexture);

            pContext->UnBindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);
          } else {
            // Draw the icon
            pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectIcon);

            opengl::cTexture* pTexture = pTextureMissing;

            switch (photos[i]->state) {
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

            const size_t x = i % columns;
            const size_t y = i / columns;
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
      }

      // Draw the filenames for the photos
      {
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
          pFont->PushBack(builderText, spitfire::filesystem::GetFile(photos[i]->sFilePath), colour, spitfire::math::cVec2(fNameX, fNameY), fRotationDegrees, scale);
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

  void cGtkmmOpenGLView::OnImageError(const string_t& sFilePath, IMAGE_SIZE imageSize)
  {
    LOG<<"cGtkmmOpenGLView::OnImageError \""<<sFilePath<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cGtkmmOpenGLViewImageErrorEvent* pEvent = new cGtkmmOpenGLViewImageErrorEvent(sFilePath, imageSize);
      eventQueue.AddItemToBack(pEvent);
      notifyMainThread.Notify();
    } else {
      LOG<<"cGtkmmOpenGLView::OnImageError On main thread \""<<sFilePath<<"\""<<std::endl;
      const size_t n = photos.size();
      for (size_t i = 0; i < n; i++) {
        if (photos[i]->sFilePath == sFilePath) {
          cPhotoEntry* pEntry = photos[i];
          pEntry->state = cPhotoEntry::STATE::LOADING_ERROR;
          break;
        }
      }
    }
  }

  void cGtkmmOpenGLView::OnImageLoaded(const string_t& sFilePath, IMAGE_SIZE imageSize, voodoo::cImage* pImage)
  {
    LOG<<"cGtkmmOpenGLView::OnImageLoaded \""<<sFilePath<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cGtkmmOpenGLViewImageLoadedEvent* pEvent = new cGtkmmOpenGLViewImageLoadedEvent(sFilePath, imageSize, pImage);
      eventQueue.AddItemToBack(pEvent);
      notifyMainThread.Notify();
    } else {
      LOG<<"cGtkmmOpenGLView::OnImageLoaded On main thread \""<<sFilePath<<"\""<<std::endl;
      const size_t n = photos.size();
      for (size_t i = 0; i < n; i++) {
        if (photos[i]->sFilePath == sFilePath) {
          cPhotoEntry* pEntry = photos[i];
          opengl::cTexture* pTexture = pContext->CreateTextureFromImage(*pImage);
          ASSERT(pTexture != nullptr);
          pEntry->state = cPhotoEntry::STATE::LOADED;
          pEntry->pTexture = pTexture;
          break;
        }
      }
    }
  }

  bool cGtkmmOpenGLView::OnKeyPressEvent(GdkEventKey* pEvent)
  {
    LOG<<"cGtkmmOpenGLView::OnKeyPressEvent"<<std::endl;
    switch (pEvent->keyval) {
      #ifdef BUILD_DEBUG
      case GDK_w: {
        bIsWireframe = !bIsWireframe;
        return true;
      }
      #endif
      /*case GDK_Up:
      case GDK_Left:
        choice--;
        if (choice < 0)
            choice = names.size() - 1;
        return true;
      case GDK_Down:
      case GDK_Right:
        choice++;
        if (choice >= names.size())
            choice = 0;
        return true;
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
