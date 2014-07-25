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

  class cPhotoBrowserViewController;

  class cWin32mmOpenGLViewEvent;

  // ** cWin32mmOpenGLView

  class cWin32mmOpenGLView : public win32mm::cOpenGLControl
  {
  public:
    friend class cOpenGLViewController;

    explicit cWin32mmOpenGLView(cPhotoBrowserViewController& controller);
    ~cWin32mmOpenGLView();

    opengl::cContext& GetContext();

    void Create(win32mm::cWindow& window, int idControl);
    void Destroy();

    void OnOpenGLViewChangedFolder(const string_t& sFolderPath);
    void OnOpenGLViewResized();
    void OnOpenGLViewFileFound();
    void OnOpenGLViewLoadedFileOrFolder();
    void OnOpenGLViewLoadedFilesClear();
    void OnOpenGLViewRightClick();
    void OnOpenGLViewSelectionChanged();
    void OnOpenGLViewPhotoCollageMode();
    void OnOpenGLViewSinglePhotoMode(const string_t& sFilePath);

  protected:
    void InitOpenGL(int argc, char* argv[]);
    void DestroyOpenGL();

    void ResizeWidget(size_t width, size_t height);

    virtual void OnSize() override;
    virtual void OnPaint() override;

    cPhotoBrowserViewController& controller;

    cWin32mmOpenGLContext context;
  };
}

#endif // WIN32_OPENGL_VIEW_H
