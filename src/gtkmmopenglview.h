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

// libopenglmm headers
#include <libopenglmm/cFont.h>
#include <libopenglmm/cSystem.h>

// Diesel headers
#include "diesel.h"

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
      LOADED
    };

    string_t sFilePath;
    STATE state;
    opengl::cTexture* pTexture;
  };

  class cGtkmmPhotoBrowser;

  class cGtkmmOpenGLView : public Gtk::DrawingArea
  {
  public:
    friend class cGtkmmPhotoBrowser;

    explicit cGtkmmOpenGLView(cGtkmmPhotoBrowser& parent);
    ~cGtkmmOpenGLView();

    void Init(int argc, char* argv[]);

    size_t GetPageHeight() const;
    size_t GetRequiredHeight() const;

    float GetScale() const;
    void SetScale(float fScale);

    void OnScrollBarScrolled(float fValue);

  protected:
    bool OnKeyPressEvent(GdkEventKey* event);

    bool OnMouseDown(int button, int x, int y);
    bool OnMouseRelease(int button, int x, int y);
    bool OnMouseScrollUp(int x, int y);
    bool OnMouseScrollDown(int x, int y);
    bool OnMouseMove(int x, int y);

  private:
    const GtkWidget* GetWidget() const;
    GtkWidget* GetWidget();

    void UpdateColumnsPageHeightAndRequiredHeight();

    bool GetPhotoAtPoint(size_t& index, const spitfire::math::cVec2& point) const;

    void CreateVertexBufferObjectSquare(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight);
    void CreateVertexBufferObjectIcon();
    void CreateVertexBufferObjectPhoto();

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

    void InitOpenGL(int argc, char* argv[]);
    void DestroyOpenGL();

    void CreateResources();
    void DestroyResources();

    void ResizeWidget(size_t width, size_t height);

    void DrawScene();

    static gboolean configure_cb(GtkWidget* pWidget, GdkEventConfigure* event, gpointer pUserData);
    static gboolean idle_cb(gpointer pUserData);

    cGtkmmPhotoBrowser& parent;

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

    opengl::cShader* pShaderPhoto;
    opengl::cShader* pShaderIcon;

    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto;
    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectIcon;

    // Text
    opengl::cFont* pFont;

    // Photos
    std::vector<cPhotoEntry*> photos;
  };
}

#endif // GTK_OPENGL_VIEW_H
