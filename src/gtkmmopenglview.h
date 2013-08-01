#ifndef GTK_OPENGL_VIEW_H
#define GTK_OPENGL_VIEW_H

// Standard headers
#include <vector>

// OpenGL headers
#include <GL/gl.h>

// GtkGLExt headers
#include <gtk/gtkgl.h>

// Gtkmm headers
#include <gtkmm/drawingarea.h>

// libgtkmm headers
#include <libgtkmm/dispatcher.h>

// libopenglmm headers
#include <libopenglmm/cFont.h>
#include <libopenglmm/cSystem.h>

#include <spitfire/util/signalobject.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "imageloadthread.h"

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

    string_t sFilePath;
    STATE state;
    opengl::cTexture* pTexturePhoto;
    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto;
    bool bIsSelected;
  };

  class cGtkmmPhotoBrowser;

  class cGtkmmOpenGLViewEvent;
  class cGtkmmOpenGLViewFolderFoundEvent;
  class cGtkmmOpenGLViewImageLoadingEvent;
  class cGtkmmOpenGLViewImageLoadedEvent;
  class cGtkmmOpenGLViewImageErrorEvent;

  class cGtkmmOpenGLView : public Gtk::DrawingArea, public cImageLoadHandler
  {
  public:
    friend class cGtkmmOpenGLViewFolderFoundEvent;
    friend class cGtkmmOpenGLViewImageLoadingEvent;
    friend class cGtkmmOpenGLViewImageLoadedEvent;
    friend class cGtkmmOpenGLViewImageErrorEvent;
    friend class cGtkmmPhotoBrowser;

    explicit cGtkmmOpenGLView(cGtkmmPhotoBrowser& parent);
    ~cGtkmmOpenGLView();

    void Init(int argc, char* argv[]);
    void Destroy();

    void SetSelectionColour(const spitfire::math::cColour& colour);

    string_t GetFolder() const;
    void SetFolder(const string_t& sFolderPath);

    size_t GetPageHeight() const;
    size_t GetRequiredHeight() const;

    float GetScale() const;
    void SetScale(float fScale);

    size_t GetPhotoCount() const;
    size_t GetLoadedPhotoCount() const;
    size_t GetSelectedPhotoCount() const;

    void StopLoading();

    void OnScrollBarScrolled(float fValue);

  protected:
    bool OnKeyPressEvent(GdkEventKey* event);

    bool OnMouseDown(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseRelease(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseDoubleClick(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseScrollUp(int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseScrollDown(int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseMove(int x, int y, bool bKeyControl, bool bKeyShift);

  private:
    const GtkWidget* GetWidget() const;
    GtkWidget* GetWidget();

    void UpdateColumnsPageHeightAndRequiredHeight();

    bool GetPhotoAtPoint(size_t& index, const spitfire::math::cVec2& point) const;

    void CreateVertexBufferObjectSelectionRectangle(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectSquare(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight);
    void CreateVertexBufferObjectIcon();
    void CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto, size_t textureWidth, size_t textureHeight);

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

    void InitOpenGL(int argc, char* argv[]);
    void DestroyOpenGL();

    void CreateResources();
    void DestroyResources();

    void CreatePhotos();
    void DestroyPhotos();

    void ResizeWidget(size_t width, size_t height);

    void RenderPhoto(size_t index, const spitfire::math::cMat4& matScale);

    void DrawScene();

    static gboolean configure_cb(GtkWidget* pWidget, GdkEventConfigure* event, gpointer pUserData);
    static gboolean idle_cb(gpointer pUserData);

    virtual void OnFolderFound(const string_t& sFolderPath) override;
    virtual void OnImageLoading(const string_t& sFilePath) override;
    virtual void OnImageLoaded(const string_t& sFilePath, IMAGE_SIZE imageSize, voodoo::cImage* pImage) override;
    virtual void OnImageError(const string_t& sFilePath, IMAGE_SIZE imageSize) override;

    void OnNotify();

    cGtkmmPhotoBrowser& parent;

    cImageLoadThread imageLoadThread;

    string_t sFolderPath;

    bool bIsWireframe;

    size_t pageHeight;
    size_t requiredHeight;

    size_t columns;

    float fScale;
    float fScrollPosition;

    opengl::cSystem system;

    opengl::cResolution resolution;

    opengl::cContext* pContext;

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

    bool bIsConfigureCalled;

    // Photos
    std::vector<cPhotoEntry*> photos;

    spitfire::math::cColour colourSelected;

    bool bIsModeSinglePhoto;
    size_t currentSinglePhoto;

    gtkmm::cGtkmmNotifyMainThread notifyMainThread;
    spitfire::util::cSignalObject soAction;
    spitfire::util::cThreadSafeQueue<cGtkmmOpenGLViewEvent> eventQueue;
  };
}

#endif // GTK_OPENGL_VIEW_H
