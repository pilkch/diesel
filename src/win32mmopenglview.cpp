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
#include "win32mmopenglview.h"
#include "photobrowserviewcontroller.h"

namespace diesel
{
  // ** cWin32mmOpenGLContext

  cWin32mmOpenGLContext::cWin32mmOpenGLContext() :
    control(NULL),
    hDC(NULL),
    hRC(NULL),
    pContext(nullptr)
  {
  }

  cWin32mmOpenGLContext::~cWin32mmOpenGLContext()
  {
    std::cout<<"cWin32mmOpenGLContext::~cWin32mmOpenGLContext"<<std::endl;
    ASSERT(!IsValid());
  }

  bool cWin32mmOpenGLContext::Create(HWND _control)
  {
    if (control != NULL) return true;

    control = _control;

    hDC = ::GetDC(control);
    ASSERT(hDC != NULL);

    const PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,                               // Version Number
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,                   // Request RGBA Format
      16,                              // Select Color Depth
      0, 0, 0, 0, 0, 0,                // Color Bits Ignored
      0,                               // No Alpha bits
      0,                               // Shift Bit Ignored
      0,                               // No Accumulation Buffer
      0, 0, 0, 0,                      // Accumulation Bits Ignored
      16,                              // 16Bit Z-Buffer (Depth Buffer)
      0,                               // No Stencil Buffer
      0,                               // No Auxiliary Buffer
      PFD_MAIN_PLANE,                  // Main Drawing Layer
      0,                               // Reserved
      0, 0, 0                          // Layer Masks Ignored
    };

    const GLuint PixelFormat = ::ChoosePixelFormat(hDC, &pfd);
    ASSERT(PixelFormat != 0);

    const bool bIsSetPixelFormat = (::SetPixelFormat(hDC, PixelFormat, &pfd) == TRUE);
    ASSERT(bIsSetPixelFormat);

    // Create OpenGL rendering context
    hRC = wglCreateContext(hDC);
    ASSERT(hDC != NULL);

    const bool bIsSetCurrentContext = (wglMakeCurrent(hDC, hRC) == TRUE);
    ASSERT(bIsSetCurrentContext);

    const int iMajor = 3;
    const int iMinor = 3;
    if (gl3wInit()) {
      LOGERROR<<"cWin32mmOpenGLContext::_SetWindowVideoMode Failed to initialize OpenGL"<<std::endl;
      return false;
    }
    if (!gl3wIsSupported(iMajor, iMinor)) {
      LOGERROR<<TEXT("cWin32mmOpenGLContext::_SetWindowVideoMode OpenGL ")<<spitfire::string::ToString(iMajor)<<TEXT(".")<<spitfire::string::ToString(iMinor)<<TEXT(" not supported")<<std::endl;
      return false;
    }

    // Set up our default clearing, projection etc.
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::cout<<"OpenGL Version: "<<(const char*)glGetString(GL_VERSION)<<", OpenGL Vendor: "<<(const char*)glGetString(GL_VENDOR)<<", OpenGL Renderer: "<<(const char*)glGetString(GL_RENDERER)<<std::endl;


    // Set our resolution
    resolution.width = 500;
    resolution.height = 500;
    resolution.pixelFormat = opengl::PIXELFORMAT::R8G8B8A8;
    // TODO: Get the size of the control

    // Init libopenglmm
    pContext = system.CreateSharedContextForControl(control, resolution);

    return (bIsSetPixelFormat && bIsSetCurrentContext);
  }

  void cWin32mmOpenGLContext::Destroy()
  {
    // Destroy our context
    if (pContext != nullptr) {
      system.DestroyContext(pContext);
      pContext = nullptr;
    }

    // Release OpenGL rendering context
    if (hRC != NULL) {
      if (!wglMakeCurrent(NULL, NULL)) {
        std::cout<<"Release of DC and RC failed"<<std::endl;
      }
      if (!wglDeleteContext(hRC)) {
        std::cout<<"Release of Rendering Context failed"<<std::endl;
      }
      hRC = NULL;
    }

    // Release Device context
    if (hDC != NULL) {
      if (ReleaseDC(control, hDC) != 1) {
        std::cout<<"Release of Device Context failed"<<std::endl;
      }
      hDC = NULL;
    }

    // Make sure we do not refer to this control in the future
    control = NULL;
  }

  bool cWin32mmOpenGLContext::IsValid() const
  {
    return (
      (hDC != NULL) &&
      (hRC != NULL) &&
      ((pContext != nullptr) && pContext->IsValid())
    );
  }

  void cWin32mmOpenGLContext::Resize(size_t width, size_t height)
  {
    LOG<<"cWin32mmOpenGLContext::Resize ("<<width<<"x"<<height<<")"<<std::endl;

    // Update our resolution
    resolution.width = width;
    resolution.height = height;

    // Resize the context
    pContext->ResizeWindow(resolution);
  }

  void cWin32mmOpenGLContext::Begin()
  {
    ASSERT(IsValid());

    bool bIsSetCurrentContext = (wglMakeCurrent(hDC, hRC) == TRUE);
    ASSERT(bIsSetCurrentContext);
  }

  void cWin32mmOpenGLContext::End()
  {
    ASSERT(IsValid());

    ::SwapBuffers(hDC);
  }

  opengl::cContext& cWin32mmOpenGLContext::GetContext()
  {
    return *pContext;
  }


  // ** cWin32mmOpenGLView

  cWin32mmOpenGLView::cWin32mmOpenGLView(cPhotoBrowserViewController& _controller) :
    controller(_controller)
  {
  }

  cWin32mmOpenGLView::~cWin32mmOpenGLView()
  {
  }

  opengl::cContext& cWin32mmOpenGLView::GetContext()
  {
    return context.GetContext();
  }

  void cWin32mmOpenGLView::Create(win32mm::cWindow& parent, int idControl)
  {
    win32mm::cOpenGLControl::Create(parent, idControl);

    context.Create(GetHandle());

    controller.SetContext(context.GetContext());
  }

  void cWin32mmOpenGLView::Destroy()
  {
    context.Destroy();
  }

  void cWin32mmOpenGLView::OnSize()
  {
    context.Resize(GetWidth(), GetHeight());
  }

  void cWin32mmOpenGLView::OnPaint()
  {
    context.Begin();

    controller.OnPaint();

    context.End();
  }
}
