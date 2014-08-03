#ifndef DIESEL_PHOTOBROWSERVIEWCONTROLLER_H
#define DIESEL_PHOTOBROWSERVIEWCONTROLLER_H

// Standard headers
#include <vector>

// OpenGL headers
#include <GL/gl.h>

// libopenglmm headers
#include <libopenglmm/cFont.h>
#include <libopenglmm/cSystem.h>

// libwin32mm headers
#include <libwin32mm/openglcontrol.h>
#include <libwin32mm/notify.h>

// Spitfire headers
#include <spitfire/util/signalobject.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "imageloadthread.h"
#ifdef __WIN__
#include "win32mmopenglview.h"
#else
#include "gtkmmopenglview.h"
#endif

namespace diesel
{
  class cPhotoEntry
  {
  public:
    cPhotoEntry();

    enum class STATE {
      NOT_FOUND,
      FOLDER,
      LOADING,
      LOADED,
      LOADING_ERROR,
    };

    string_t sFileNameNoExtension;
    STATE state;
    bool bLoadingFull;
    opengl::cTexture* pTexturePhotoThumbnail;
    opengl::cTexture* pTexturePhotoFull;
    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhotoThumbnail;
    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhotoFull;
    bool bIsSelected;
  };

  class cPhotoBrowserViewControllerEvent;
  class cPhotoBrowserViewControllerFolderFoundEvent;
  class cPhotoBrowserViewControllerFileFoundEvent;
  class cPhotoBrowserViewControllerImageLoadedEvent;
  class cPhotoBrowserViewControllerImageErrorEvent;

  class cPhotoBrowserViewController : public cImageLoadHandler
  {
  public:
    friend class cPhotoBrowserViewControllerFolderFoundEvent;
    friend class cPhotoBrowserViewControllerFileFoundEvent;
    friend class cPhotoBrowserViewControllerImageLoadedEvent;
    friend class cPhotoBrowserViewControllerImageErrorEvent;
    friend class cWin32mmPhotoBrowser;

    explicit cPhotoBrowserViewController(cWin32mmOpenGLView& view);
    ~cPhotoBrowserViewController();

    void Init(opengl::cContext& context);
    void Destroy();

    void SetCurrentFolderPath(const string_t& sFolderPath);

    void SetSelectionColour(const spitfire::math::cColour& colour);

    size_t GetRequiredHeight() const;

    float GetScale() const;
    void SetScale(float fScale);

    void SetCacheMaximumSizeGB(size_t nCacheMaximumSizeGB);

    size_t GetPhotoCount() const;
    size_t GetLoadedPhotoCount() const;
    size_t GetSelectedPhotoCount() const;

    void ResizeWidget(size_t width, size_t height);

    void StopLoading();

    void OnScrollBarScrolled(float fValue);

    void OnPaint();

    //bool OnKeyPressEvent(GdkEventKey* event);

    bool OnMouseMove(int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseDown(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseRelease(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseDoubleClick(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseScrollUp(int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseScrollDown(int x, int y, bool bKeyControl, bool bKeyShift);

  private:
    void CreateVertexBufferObjectSelectionRectangle(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectSquare(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fX, float fY, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight);
    void CreateVertexBufferObjectIcon();
    void CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto, size_t textureWidth, size_t textureHeight);

    void ClampScrollBarPosition();
    void UpdateColumnsPageHeightAndRequiredHeight();

    bool GetPhotoAtPoint(size_t& index, const spitfire::math::cVec2& point) const;

    void PreloadSinglePhoto(size_t index);

    void SetSinglePhotoMode(size_t index);
    void SetPhotoCollageMode();

    void CreateResources();
    void DestroyResources();

    void CreatePhotos();
    void DestroyPhotos();

    void RenderPhoto(size_t index, const spitfire::math::cMat4& matScale);

    virtual void OnFolderFound(const string_t& sFolderName) override;
    virtual void OnFileFound(const string_t& sFileNameNoExtension) override;
    virtual void OnImageLoaded(const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, voodoo::cImage* pImage) override;
    virtual void OnImageError(const string_t& sFileNameNoExtension) override;

    cWin32mmOpenGLView& view;

    opengl::cResolution resolution;

    opengl::cContext* pContext;

    cImageLoadThread imageLoadThread;

    string_t sFolderPath;

    #ifdef BUILD_DEBUG
    bool bIsWireframe;
    #endif

    size_t pageHeight;
    size_t requiredHeight;

    size_t columns;

    float fScale;
    float fScrollPosition;

    opengl::cTexture* pTextureMissing;
    opengl::cTexture* pTextureFolder;
    opengl::cTexture* pTextureLoading;
    opengl::cTexture* pTextureLoadingError;

    opengl::cShader* pShaderSelectionRectangle;
    opengl::cShader* pShaderPhoto;
    opengl::cShader* pShaderIcon;

    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectSelectionRectangle;
    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectIcon;

    // Text
    opengl::cFont* pFont;

    // Photos
    std::vector<cPhotoEntry*> photos;

    // Selection
    spitfire::math::cColour colourSelected;

    bool bIsModeSinglePhoto;
    size_t currentSinglePhoto;

    #ifdef __WIN__
    win32mm::cRunOnMainThread<cPhotoBrowserViewController, cPhotoBrowserViewControllerEvent> notifyMainThread;
    #else
    // TODO: Rename cGtkmmRunOnMainThread to cRunOnMainThread
    gtkmm::cGtkmmRunOnMainThread<cGtkmmOpenGLView, cGtkmmOpenGLViewEvent> notifyMainThread;
    #endif
  };
}

#endif // DIESEL_PHOTOBROWSERVIEWCONTROLLER_H
