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
#include <libopenglmm/cSystem.h>

namespace diesel
{
  class cGtkmmMainWindow;

  class cGtkmmOpenGLView : public Gtk::DrawingArea
  {
  public:
    friend class cGtkmmMainWindow;

    explicit cGtkmmOpenGLView(cGtkmmMainWindow& parent);
    ~cGtkmmOpenGLView();

    void Init(int argc, char* argv[]);

  protected:
    virtual bool on_key_press_event(GdkEventKey* event) override;

  private:
    const GtkWidget* GetWidget() const;
    GtkWidget* GetWidget();

    void CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pObject, size_t textureWidth, size_t textureHeight);

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

    void InitOpenGL(int argc, char* argv[]);
    void DestroyOpenGL();

    void CreateResources();
    void DestroyResources();

    void ResizeWidget(size_t width, size_t height);

    void DrawScene();

    static gboolean configure_cb(GtkWidget* pWidget, GdkEventConfigure* event, gpointer pUserData);
    static gboolean idle_cb(gpointer pUserData);

    cGtkmmMainWindow& parent;

    bool bIsWireframe;

    spitfire::math::cVec2 scale;

    opengl::cSystem system;

    opengl::cResolution resolution;

    opengl::cContext* pContext;

    std::vector<opengl::cTexture*> photoTextures;
    opengl::cShader* pShaderPhoto;
    opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto;
  };
}

#endif // GTK_OPENGL_VIEW_H
