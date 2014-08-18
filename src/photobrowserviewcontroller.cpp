// Standard headers
#include <cassert>
#include <iostream>
#include <limits>

// OpenGL headers
#include <GL/GLee.h>
#include <GL/glu.h>

// libopenglmm headers
#include <libopenglmm/cContext.h>
#include <libopenglmm/cGeometry.h>
#include <libopenglmm/cShader.h>
#include <libopenglmm/cTexture.h>
#include <libopenglmm/cVertexBufferObject.h>

// Spitfire headers
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/log.h>

// Diesel headers
#include "photobrowserviewcontroller.h"

namespace diesel
{
  class cPhotoBrowserViewControllerEvent
  {
  public:
    virtual ~cPhotoBrowserViewControllerEvent() {}

    virtual void EventFunction(cPhotoBrowserViewController& view) = 0;
  };


  class cPhotoBrowserViewControllerFolderFoundEvent : public cPhotoBrowserViewControllerEvent
  {
  public:
    cPhotoBrowserViewControllerFolderFoundEvent(const string_t& sFolderName);

    virtual void EventFunction(cPhotoBrowserViewController& view) override;

    string_t sFolderName;
  };

  cPhotoBrowserViewControllerFolderFoundEvent::cPhotoBrowserViewControllerFolderFoundEvent(const string_t& _sFolderName) :
    sFolderName(_sFolderName)
  {
  }

  void cPhotoBrowserViewControllerFolderFoundEvent::EventFunction(cPhotoBrowserViewController& view)
  {
    view.OnFolderFound(sFolderName);
  }


  class cPhotoBrowserViewControllerFileFoundEvent : public cPhotoBrowserViewControllerEvent
  {
  public:
    cPhotoBrowserViewControllerFileFoundEvent(const string_t& sFileNameNoExtension);

    virtual void EventFunction(cPhotoBrowserViewController& view) override;

    string_t sFileNameNoExtension;
  };

  cPhotoBrowserViewControllerFileFoundEvent::cPhotoBrowserViewControllerFileFoundEvent(const string_t& _sFileNameNoExtension) :
    sFileNameNoExtension(_sFileNameNoExtension)
  {
  }

  void cPhotoBrowserViewControllerFileFoundEvent::EventFunction(cPhotoBrowserViewController& view)
  {
    view.OnFileFound(sFileNameNoExtension);
  }


  class cPhotoBrowserViewControllerImageLoadedEvent : public cPhotoBrowserViewControllerEvent
  {
  public:
    cPhotoBrowserViewControllerImageLoadedEvent(const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, voodoo::cImage* pImage);
    ~cPhotoBrowserViewControllerImageLoadedEvent();

    virtual void EventFunction(cPhotoBrowserViewController& view) override;

    string_t sFileNameNoExtension;
    IMAGE_SIZE imageSize;
    voodoo::cImage* pImage;
  };

  cPhotoBrowserViewControllerImageLoadedEvent::cPhotoBrowserViewControllerImageLoadedEvent(const string_t& _sFileNameNoExtension, IMAGE_SIZE _imageSize, voodoo::cImage* _pImage) :
    sFileNameNoExtension(_sFileNameNoExtension),
    imageSize(_imageSize),
    pImage(_pImage)
  {
  }

  cPhotoBrowserViewControllerImageLoadedEvent::~cPhotoBrowserViewControllerImageLoadedEvent()
  {
    spitfire::SAFE_DELETE(pImage);
  }

  void cPhotoBrowserViewControllerImageLoadedEvent::EventFunction(cPhotoBrowserViewController& view)
  {
    view.OnImageLoaded(sFileNameNoExtension, imageSize, pImage);
  }


  class cPhotoBrowserViewControllerImageErrorEvent : public cPhotoBrowserViewControllerEvent
  {
  public:
    explicit cPhotoBrowserViewControllerImageErrorEvent(const string_t& sFileNameNoExtension);

    virtual void EventFunction(cPhotoBrowserViewController& view) override;

    string_t sFileNameNoExtension;
  };

  cPhotoBrowserViewControllerImageErrorEvent::cPhotoBrowserViewControllerImageErrorEvent(const string_t& _sFileNameNoExtension) :
    sFileNameNoExtension(_sFileNameNoExtension)
  {
  }

  void cPhotoBrowserViewControllerImageErrorEvent::EventFunction(cPhotoBrowserViewController& view)
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


  // ** cPhotoBrowserViewController

  cPhotoBrowserViewController::cPhotoBrowserViewController(cWin32mmOpenGLView& _view) :
    view(_view),
    pContext(nullptr),
    imageLoadThread(*this),
    bIsWireframe(false),
    pageHeight(500),
    requiredHeight(12000),
    columns(10),
    fScale(1.0f),
    fScrollPosition(0.0f),
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
    colourSelected(1.0f, 1.0f, 1.0f),
    bIsModeSinglePhoto(false),
    currentSinglePhoto(0),
    notifyMainThread(*this)
  {
    // Set our resolution
    resolution.width = 500;
    resolution.height = 500;
    resolution.pixelFormat = opengl::PIXELFORMAT::R8G8B8A8;

    UpdateColumnsPageHeightAndRequiredHeight();

    imageLoadThread.Start();

    notifyMainThread.Create();
  }

  cPhotoBrowserViewController::~cPhotoBrowserViewController()
  {
    Destroy();
  }

  void cPhotoBrowserViewController::Init(opengl::cContext& context)
  {
    pContext = &context;

    CreateResources();
  }

  void cPhotoBrowserViewController::Destroy()
  {
    imageLoadThread.StopNow();

    // Destroy any further events
    notifyMainThread.ClearEventQueue();

    DestroyResources();
  }

  void cPhotoBrowserViewController::SetCurrentFolderPath(const string_t& _sFolderPath)
  {
    if (sFolderPath != _sFolderPath) {
      // Clear the request queue as soon as possible so that we don't waste time loading images from the old folder
      imageLoadThread.StopLoading();

      sFolderPath = _sFolderPath;

      // Reload our photos
      DestroyPhotos();
      CreatePhotos();

      view.Update();
    }
  }

  void cPhotoBrowserViewController::SetSelectionColour(const spitfire::math::cColour& colour)
  {
    colourSelected = colour;
  }

  void cPhotoBrowserViewController::SetCacheMaximumSizeGB(size_t nCacheMaximumSizeGB)
  {
    imageLoadThread.SetMaximumCacheSizeGB(nCacheMaximumSizeGB);
  }

  void cPhotoBrowserViewController::StopLoading()
  {
    imageLoadThread.StopLoading();

    std::vector<cPhotoEntry*>::iterator iter = photos.begin();
    const std::vector<cPhotoEntry*>::iterator iterEnd = photos.end();
    while (iter != iterEnd) {
      if ((*iter)->state == cPhotoEntry::STATE::LOADING) (*iter)->state = cPhotoEntry::STATE::LOADING_ERROR;

      iter++;
    }

    view.OnOpenGLViewLoadedFileOrFolder();
  }

  size_t cPhotoBrowserViewController::GetPhotoCount() const
  {
    return photos.size();
  }

  size_t cPhotoBrowserViewController::GetLoadedPhotoCount() const
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

  size_t cPhotoBrowserViewController::GetSelectedPhotoCount() const
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

  size_t cPhotoBrowserViewController::GetRequiredHeight() const
  {
    return requiredHeight;
  }

  float cPhotoBrowserViewController::GetScale() const
  {
    return fScale;
  }

  void cPhotoBrowserViewController::SetScale(float _fScale)
  {
    fScale = _fScale;

    UpdateColumnsPageHeightAndRequiredHeight();
  }

  void cPhotoBrowserViewController::ClampScrollBarPosition()
  {
    const float fMax = max(0.0f, float(requiredHeight) / fScale);
    fScrollPosition = spitfire::math::clamp(fScrollPosition, 0.0f, fMax);
  }

  void cPhotoBrowserViewController::UpdateColumnsPageHeightAndRequiredHeight()
  {
    const float fWidth = (resolution.width - fThumbNailSpacing);

    columns = max<size_t>(1, (fWidth / (fThumbNailWidth + fThumbNailSpacing)) / fScale);

    const size_t rows = max<size_t>(1, spitfire::math::RoundUpToNearestInt(float(photos.size()) / float(columns)));
    const float fRequiredHeight = fThumbNailSpacing + (float(rows) * (fThumbNailHeight + fThumbNailSpacing));

    requiredHeight = fRequiredHeight * fScale;

    pageHeight = resolution.height * fScale;

    LOG<<"cPhotoBrowserViewController::UpdateColumnsPageHeightAndRequiredHeight fScale="<<fScale<<", photos="<<photos.size()<<", rows="<<rows<<", columns="<<columns<<", requiredHeight="<<requiredHeight<<", pageHeight="<<pageHeight<<std::endl;
  }

  bool cPhotoBrowserViewController::GetPhotoAtPoint(size_t& index, const spitfire::math::cVec2& _point) const
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
        LOG<<"cPhotoBrowserViewController::GetPhotoAtPoint In item "<<i<<std::endl;
        index = i;
        return true;
      }
    }

    LOG<<"cPhotoBrowserViewController::GetPhotoAtPoint In blank space"<<std::endl;
    return false;
  }

  void cPhotoBrowserViewController::CreateVertexBufferObjectSelectionRectangle(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight)
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

    pStaticVertexBufferObject->Compile2D();
  }

  void cPhotoBrowserViewController::CreateVertexBufferObjectSquare(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight)
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

    pStaticVertexBufferObject->Compile2D();
  }

  void cPhotoBrowserViewController::CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fX, float fY, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight)
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

    pStaticVertexBufferObject->Compile2D();
  }

  void cPhotoBrowserViewController::CreateVertexBufferObjectIcon()
  {
    pStaticVertexBufferObjectIcon = pContext->CreateStaticVertexBufferObject();
    ASSERT(pStaticVertexBufferObjectIcon != nullptr);
    const float fWidthAndHeight = min(fThumbNailWidth, fThumbNailHeight);
    CreateVertexBufferObjectSquare(pStaticVertexBufferObjectIcon, fWidthAndHeight, fWidthAndHeight);
  }

  void cPhotoBrowserViewController::CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto, size_t textureWidth, size_t textureHeight)
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

  /*void cPhotoBrowserViewController::CreateVertexBufferObjectPhotos()
  {
    if (pTexture != nullptr) {

    }
  }*/

  void cPhotoBrowserViewController::CreateResources()
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

    pTextureMissing = pContext->CreateTexture(TEXT("data/textures/icon_question_mark.png"));
    ASSERT(pTextureMissing != nullptr);
    pTextureFolder = pContext->CreateTexture(TEXT("data/textures/icon_folder.png"));
    ASSERT(pTextureFolder != nullptr);
    pTextureLoading = pContext->CreateTexture(TEXT("data/textures/icon_stopwatch.png"));
    ASSERT(pTextureLoading != nullptr);
    pTextureLoadingError = pContext->CreateTexture(TEXT("data/textures/icon_loading_error.png"));
    ASSERT(pTextureLoadingError != nullptr);
  }

  void cPhotoBrowserViewController::DestroyResources()
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

  void cPhotoBrowserViewController::CreatePhotos()
  {
    // If we don't have a folder to show then we can just return
    if (sFolderPath.empty()) return;

    // Reset our files loaded and total count
    view.OnOpenGLViewLoadedFilesClear();

    // Tell our image loading thread to start loading the folder
    imageLoadThread.LoadFolderThumbnails(sFolderPath);
  }

  void cPhotoBrowserViewController::DestroyPhotos()
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

  void cPhotoBrowserViewController::ResizeWidget(size_t width, size_t height)
  {
    resolution.width = width;
    resolution.height = height;
    pContext->ResizeWindow(resolution);

    UpdateColumnsPageHeightAndRequiredHeight();

    view.OnOpenGLViewResized();
  }

  void cPhotoBrowserViewController::RenderPhoto(size_t index, const spitfire::math::cMat4& matScale)
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

  void cPhotoBrowserViewController::OnPaint()
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

          pVBOText->Compile2D();

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

          pVBOText->Compile2D();

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

  void cPhotoBrowserViewController::OnFolderFound(const string_t& sFolderName)
  {
    LOG<<"cPhotoBrowserViewController::OnFolderFound \""<<sFolderName<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cPhotoBrowserViewControllerFolderFoundEvent* pEvent = new cPhotoBrowserViewControllerFolderFoundEvent(sFolderName);
      notifyMainThread.PushEventToMainThread(pEvent);
    } else {
      LOG<<"cPhotoBrowserViewController::OnFolderFound On main thread \""<<sFolderName<<"\""<<std::endl;
      cPhotoEntry* pEntry = new cPhotoEntry;
      pEntry->sFileNameNoExtension = sFolderName;
      pEntry->state = cPhotoEntry::STATE::FOLDER;
      photos.push_back(pEntry);

      view.OnOpenGLViewLoadedFileOrFolder(); // A folder counts as a loaded file
    }
  }

  void cPhotoBrowserViewController::OnFileFound(const string_t& sFileNameNoExtension)
  {
    LOG<<"cPhotoBrowserViewController::OnFileFound \""<<sFileNameNoExtension<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cPhotoBrowserViewControllerFileFoundEvent* pEvent = new cPhotoBrowserViewControllerFileFoundEvent(sFileNameNoExtension);
      notifyMainThread.PushEventToMainThread(pEvent);
    } else {
      LOG<<"cPhotoBrowserViewController::OnFileFound On main thread \""<<sFileNameNoExtension<<"\""<<std::endl;
      cPhotoEntry* pEntry = new cPhotoEntry;
      pEntry->sFileNameNoExtension = sFileNameNoExtension;
      pEntry->state = cPhotoEntry::STATE::LOADING;
      photos.push_back(pEntry);

      view.OnOpenGLViewFileFound();
    }
  }

  void cPhotoBrowserViewController::OnImageError(const string_t& sFileNameNoExtension)
  {
    LOG<<"cPhotoBrowserViewController::OnImageError \""<<sFileNameNoExtension<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cPhotoBrowserViewControllerImageErrorEvent* pEvent = new cPhotoBrowserViewControllerImageErrorEvent(sFileNameNoExtension);
      notifyMainThread.PushEventToMainThread(pEvent);
    } else {
      LOG<<"cPhotoBrowserViewController::OnImageError On main thread \""<<sFileNameNoExtension<<"\""<<std::endl;
      const size_t n = photos.size();
      for (size_t i = 0; i < n; i++) {
        if (photos[i]->sFileNameNoExtension == sFileNameNoExtension) {
          cPhotoEntry* pEntry = photos[i];
          pEntry->state = cPhotoEntry::STATE::LOADING_ERROR;
          break;
        }
      }

      view.OnOpenGLViewLoadedFileOrFolder();
    }
  }

  void cPhotoBrowserViewController::OnImageLoaded(const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, voodoo::cImage* pImage)
  {
    LOG<<"cPhotoBrowserViewController::OnImageLoaded \""<<sFileNameNoExtension<<"\""<<std::endl;

    if (!spitfire::util::IsMainThread()) {
      cPhotoBrowserViewControllerImageLoadedEvent* pEvent = new cPhotoBrowserViewControllerImageLoadedEvent(sFileNameNoExtension, imageSize, pImage);
      notifyMainThread.PushEventToMainThread(pEvent);
    } else {
      LOG<<"cPhotoBrowserViewController::OnImageLoaded On main thread \""<<sFileNameNoExtension<<"\""<<std::endl;
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

      view.OnOpenGLViewLoadedFileOrFolder();
    }
  }

  void cPhotoBrowserViewController::PreloadSinglePhoto(size_t index)
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

  void cPhotoBrowserViewController::SetSinglePhotoMode(size_t index)
  {
    ASSERT(index < photos.size());

    currentSinglePhoto = index;

    // Load this photo
    PreloadSinglePhoto(currentSinglePhoto);

    // Load the previous and next photos
    if (currentSinglePhoto != 0) PreloadSinglePhoto(currentSinglePhoto - 1);
    if ((currentSinglePhoto + 1) < photos.size()) PreloadSinglePhoto(currentSinglePhoto + 1);

    // Notify the view
    view.OnOpenGLViewSinglePhotoMode(photos[currentSinglePhoto]->sFileNameNoExtension);
  }

  void cPhotoBrowserViewController::SetPhotoCollageMode()
  {
    // Notify the view
    view.OnOpenGLViewPhotoCollageMode();
  }

#if 0
  bool cPhotoBrowserViewController::OnKeyPressEvent(GdkEventKey* pEvent)
  {
    LOG<<"cPhotoBrowserViewController::OnKeyPressEvent"<<std::endl;

    // Handle single photo mode
    if (bIsModeSinglePhoto) {
      switch (pEvent->keyval) {
        case GDK_Left:
        case GDK_Up:
        case GDK_Page_Up: {
          if (currentSinglePhoto != 0) SetSinglePhotoMode(currentSinglePhoto - 1);
          return true;
        }
        case GDK_Right:
        case GDK_Down:
        case GDK_Page_Down:
        case GDK_space: {
          if (currentSinglePhoto + 1 < photos.size()) SetSinglePhotoMode(currentSinglePhoto + 1);
          return true;
        }
        case GDK_Home: {
          SetSinglePhotoMode(0);
          return true;
        }
        case GDK_End: {
          if (!photos.empty()) SetSinglePhotoMode(photos.size() - 1);
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
        view.OnOpenGLViewScrollTop();
        return true;
      }
      case GDK_End: {
        view.OnOpenGLViewScrollBottom();
        return true;
      }
      case GDK_Page_Up: {
        view.OnOpenGLViewScrollPageUp();
        return true;
      }
      case GDK_Page_Down: {
        view.OnOpenGLViewScrollPageDown();
        return true;
      }
      case GDK_0: {
        if ((pEvent->state & GDK_CONTROL_MASK) != 0) {
          // Reset the zoom
          SetScale(1.0f);
          view.OnOpenGLViewContentChanged();
          return true;
        }

        break;
      }
    }

    return false;
  }
#endif

  bool cPhotoBrowserViewController::OnMouseDown(int button, int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cPhotoBrowserViewController::OnMouseDown"<<std::endl;

    // Set the focus to this widget so that arrow keys work
    //grab_focus();

    // Change the selection on left and right click
    if ((button == 1) || (button == 3)) {
      const size_t n = photos.size();

      size_t index = 0;
      if (GetPhotoAtPoint(index, spitfire::math::cVec2(x, y))) {
        LOG<<"cPhotoBrowserViewController::OnMouseDown item="<<index<<std::endl;
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

      view.OnOpenGLViewSelectionChanged();
    }

    return true;
  }

  bool cPhotoBrowserViewController::OnMouseRelease(int button, int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cPhotoBrowserViewController::OnMouseRelease"<<std::endl;

    // Handle right click
    if (button == 3) view.OnOpenGLViewRightClick();

    return true;
  }

  bool cPhotoBrowserViewController::OnMouseDoubleClick(int button, int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cPhotoBrowserViewController::OnMouseDoubleClick"<<std::endl;

    if (button == 1) {
      if (!bIsModeSinglePhoto) {
        size_t index = 0;
        if (GetPhotoAtPoint(index, spitfire::math::cVec2(x, y))) {
          ASSERT(index < photos.size());
          if (!bKeyControl && !bKeyShift) {
            if (photos[index]->state == cPhotoEntry::STATE::FOLDER) {
              // Change to this folder
              view.OnOpenGLViewChangedFolder(spitfire::filesystem::MakeFilePath(sFolderPath, photos[index]->sFileNameNoExtension));
            } else {
              // Enter single photo mode
              bIsModeSinglePhoto = true;
              SetSinglePhotoMode(index);
            }
          }
        }
      } else {
        // Exit single photo mode
        bIsModeSinglePhoto = false;
      }
    }

    return true;
  }

  bool cPhotoBrowserViewController::OnMouseScrollUp(int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cPhotoBrowserViewController::OnMouseScrollUp"<<std::endl;
    return false;
  }

  bool cPhotoBrowserViewController::OnMouseScrollDown(int x, int y, bool bKeyControl, bool bKeyShift)
  {
    LOG<<"cPhotoBrowserViewController::OnMouseScrollDown"<<std::endl;
    return false;
  }

  bool cPhotoBrowserViewController::OnMouseMove(int x, int y, bool bKeyControl, bool bKeyShift)
  {
    //LOG<<"cPhotoBrowserViewController::OnMouseMove"<<std::endl;
    return false;
  }

  void cPhotoBrowserViewController::OnScrollBarScrolled(float fValue)
  {
    LOG<<"cPhotoBrowserViewController::OnScrollBarScrolled "<<fValue<<std::endl;

    // Subtract a page because the scrollbar is strange
    fValue = max(0.0f, fValue - (float(pageHeight) / fScale));

    fScrollPosition = fValue / fScale;

    // Clamp the value to our range
    ClampScrollBarPosition();
  }
}
