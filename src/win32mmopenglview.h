#ifndef WIN32_OPENGL_VIEW_H
#define WIN32_OPENGL_VIEW_H

// Standard headers
#include <vector>

// OpenGL headers
#include <GL/gl.h>

// libopenglmm headers
#include <libopenglmm/cFont.h>
#include <libopenglmm/cSystem.h>

// libwin32mm headers
#include <libwin32mm/openglcontrol.h>

// Spitfire headers
#include <spitfire/util/signalobject.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  // ** cWin32mmOpenGLContext

  class cWin32mmOpenGLContext
  {
  public:
    cWin32mmOpenGLContext();
    ~cWin32mmOpenGLContext();

    bool Create(HWND control);
    void Destroy();

    bool IsValid() const;

    void Resize(size_t width, size_t height);

    void Begin();
    void End();

    opengl::cContext& GetContext();

  private:
    HWND control;
    HDC hDC;
    HGLRC hRC;

    opengl::cSystem system;

    opengl::cResolution resolution;

    opengl::cContext* pContext;
  };

  class cPhotoBrowser;

  class cWin32mmOpenGLViewEvent;

  // ** cWin32mmOpenGLView

  class cWin32mmOpenGLView : public win32mm::cOpenGLControl
  {
  public:
    friend class cPhotoBrowser;

    explicit cWin32mmOpenGLView(cPhotoBrowser& parent);
    ~cWin32mmOpenGLView();

    void Create(win32mm::cWindow& window, int idControl);
    void Destroy();

  protected:
    //bool OnKeyPressEvent(GdkEventKey* event);

    bool OnMouseDown(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseRelease(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseDoubleClick(int button, int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseScrollUp(int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseScrollDown(int x, int y, bool bKeyControl, bool bKeyShift);
    bool OnMouseMove(int x, int y, bool bKeyControl, bool bKeyShift);

  private:
    void CreateVertexBufferObjectSelectionRectangle(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectSquare(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fWidth, float fHeight);
    void CreateVertexBufferObjectRect(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, float fX, float fY, float fWidth, float fHeight, size_t textureWidth, size_t textureHeight);
    void CreateVertexBufferObjectIcon();
    void CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pStaticVertexBufferObjectPhoto, size_t textureWidth, size_t textureHeight);

    void InitOpenGL(int argc, char* argv[]);
    void DestroyOpenGL();

    void ResizeWidget(size_t width, size_t height);

    virtual void OnSize() override;
    virtual void OnPaint() override;

    cPhotoBrowser& parent;

    cWin32mmOpenGLContext context;
  };

  class cPhotoBrowser
  {
  public:

  };
}

#endif // WIN32_OPENGL_VIEW_H
