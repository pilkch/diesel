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

// Spitfire headers
#include <spitfire/util/signalobject.h>
#include <spitfire/util/thread.h>

// Diesel headers
#ifdef __WIN__
#include "win32mmopenglview.h"
#else
#include "gtkmmopenglview.h"
#endif

namespace diesel
{
  class cPhotoBrowserViewController
  {
  public:
    cPhotoBrowserViewController();

    void SetContext(opengl::cContext& context);

    void OnPaint();

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

    opengl::cContext* pContext;
  };
}

#endif // DIESEL_PHOTOBROWSERVIEWCONTROLLER_H
