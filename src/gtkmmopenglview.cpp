// Standard headers
#include <cassert>
#include <iostream>

// Gtk headers
#include <gdk/gdkkeysyms-compat.h>

// Cairo headers
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
#include "gtkmmmainwindow.h"
#include "gtkmmopenglview.h"

namespace diesel
{
  const float fThumbNailWidth = 100.0f;
  const float fThumbNailHeight = 100.0f;
  const float fThumbNailSpacing = 20.0f;

  // ** cGtkmmOpenGLView

  cGtkmmOpenGLView::cGtkmmOpenGLView(cGtkmmMainWindow& _parent) :
    parent(_parent),
    bIsWireframe(false),
    pageHeight(500),
    requiredHeight(12000),
    fScale(1.0f),
    fScrollPosition(0.0f),
    pContext(nullptr),
    pShaderPhoto(nullptr),
    pStaticVertexBufferObjectPhoto(nullptr),
    pFont(nullptr)
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
  }

  cGtkmmOpenGLView::~cGtkmmOpenGLView()
  {
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
  }

  void cGtkmmOpenGLView::CreateVertexBufferObjectPhoto(opengl::cStaticVertexBufferObject* pStaticVertexBufferObject, size_t textureWidth, size_t textureHeight)
  {
    ASSERT(pStaticVertexBufferObject != nullptr);

    opengl::cGeometryDataPtr pGeometryDataPtr = opengl::CreateGeometryData();

    const float fTextureWidth = textureWidth;
    const float fTextureHeight = textureHeight;

    const float fRatio = fTextureWidth / fTextureHeight;

    const float_t fWidth = fThumbNailWidth;
    const float_t fHeight = fWidth * (1.0f / fRatio);
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

  void cGtkmmOpenGLView::CreateResources()
  {
    // Return if we have already created our resources
    if (pShaderPhoto != nullptr) {
      // Recreate the vertex buffer object
      if (pStaticVertexBufferObjectPhoto != nullptr) {
        pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectPhoto);
        pStaticVertexBufferObjectPhoto = nullptr;
      }

      // Create our vertex buffer object
      pStaticVertexBufferObjectPhoto = pContext->CreateStaticVertexBufferObject();
      ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
      CreateVertexBufferObjectPhoto(pStaticVertexBufferObjectPhoto, 3040, 2024);

      return;
    }

    DestroyResources();

    // Create our textures
    for (spitfire::filesystem::cFolderIterator iter("/home/chris/Pictures/20091102 Mount Kosciuszko"); iter.IsValid(); iter.Next()) {
      if (photoTextures.size() > 16) break;

      if (iter.IsFile()) {
        opengl::cTexture* pTexture = pContext->CreateTexture(iter.GetFullPath());
        ASSERT(pTexture != nullptr);
        photoTextures.push_back(pTexture);
        photoNames.push_back(iter.GetFileOrFolder());
      }
    }

    // Create our vertex buffer object
    pStaticVertexBufferObjectPhoto = pContext->CreateStaticVertexBufferObject();
    ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
    CreateVertexBufferObjectPhoto(pStaticVertexBufferObjectPhoto, 3040, 2024);

    // Create our shader
    pShaderPhoto = pContext->CreateShader(TEXT("data/shaders/passthrough.vert"), TEXT("data/shaders/passthrough.frag"));
    ASSERT(pShaderPhoto != nullptr);

    // Create our font
    pFont = pContext->CreateFont(TEXT("data/fonts/pricedown.ttf"), 32, TEXT("data/shaders/font.vert"), TEXT("data/shaders/font.frag"));
    assert(pFont != nullptr);
    assert(pFont->IsValid());
  }

  void cGtkmmOpenGLView::DestroyResources()
  {
    if (pFont != nullptr) {
      pContext->DestroyFont(pFont);
      pFont = nullptr;
    }

    if (pStaticVertexBufferObjectPhoto != nullptr) {
      pContext->DestroyStaticVertexBufferObject(pStaticVertexBufferObjectPhoto);
      pStaticVertexBufferObjectPhoto = nullptr;
    }

    if (pShaderPhoto != nullptr) {
      pContext->DestroyShader(pShaderPhoto);
      pShaderPhoto = nullptr;
    }

    const size_t n = photoTextures.size();
    for (size_t i = 0; i < n; i++) {
      opengl::cTexture* pTexture = photoTextures[i];
      if (pTexture != nullptr) pContext->DestroyTexture(pTexture);
    }

    photoTextures.clear();
    photoNames.clear();
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

      /* Try single-buffered visual */
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

    /* Delimits the end of the OpenGL execution. */
    //gdk_gl_drawable_gl_end(gl_drawable);

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
    pageHeight = height;

    resolution.width = width;
    resolution.height = height;
    pContext->ResizeWindow(resolution);
  }

  void cGtkmmOpenGLView::DrawScene()
  {
    ASSERT(pContext != nullptr);
    ASSERT(pContext->IsValid());

    ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
    ASSERT(pStaticVertexBufferObjectPhoto->IsCompiled());

    ASSERT(pShaderPhoto != nullptr);
    ASSERT(pShaderPhoto->IsCompiledProgram());

    pContext->SetClearColour(spitfire::math::cColour(0.0f, 0.0f, 0.0f, 1.0f));

    pContext->BeginRenderToScreen();

    if (bIsWireframe) pContext->EnableWireframe();

    // Render the photos
    {
      pContext->BeginRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO);

      spitfire::math::cMat4 matModelView2D;

      spitfire::math::cMat4 matScale;
      matScale.SetScale(fScale, fScale, 1.0f);

      const float fWidth = resolution.width - (fThumbNailSpacing + fThumbNailSpacing);

      const size_t columns = fWidth / (fThumbNailWidth + fThumbNailSpacing);

      // Draw the photos
      {
        pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);


        const size_t nPhotos = photoTextures.size();
        for (size_t i = 0; i < nPhotos; i++) {
          opengl::cTexture* pTexture = photoTextures[i];

          pContext->BindTexture(0, *pTexture);

          pContext->BindShader(*pShaderPhoto);

          const size_t x = i % columns;
          const size_t y = i / columns;
          const float fX = fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing));
          const float fY = fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing));
          matModelView2D.SetTranslation(fX, fY, 0.0f);

          pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

          pContext->DrawStaticVertexBufferObjectTriangles2D(*pStaticVertexBufferObjectPhoto);

          pContext->UnBindShader(*pShaderPhoto);

          pContext->UnBindTexture(0, *pTexture);
        }

        pContext->UnBindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);
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

        const size_t n = photoNames.size();
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
          pFont->PushBack(builderText, photoNames[i], colour, spitfire::math::cVec2(fNameX, fNameY), fRotationDegrees, scale);
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
          matModelView2D.SetTranslation(0.0f, 0.0f, 0.0f);

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

    /* update control data/params  in this function if needed */
    GtkWidget* pWidget = GTK_WIDGET(pUserData);

    // Force a redraw
    GdkRectangle dummyRect = { 0, 0, 1, 1 };
    gdk_window_invalidate_rect(gdk_gl_window_get_window(gtk_widget_get_gl_window(pWidget)), &dummyRect, TRUE);

    return TRUE;
  }

  bool cGtkmmOpenGLView::on_key_press_event(GdkEventKey* event)
  {
    LOG<<"cGtkmmOpenGLView::on_key_press_event"<<std::endl;
    switch (event->keyval) {
      case GDK_Escape: {
        parent.OnMenuFileQuit();
        return true;
      }
      case GDK_w: {
        bIsWireframe = !bIsWireframe;
        return true;
      }
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
    }

    return false;
  }

  bool cGtkmmOpenGLView::OnMouseDown(int button, int x, int y)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseDown"<<std::endl;
    return false;
  }

  bool cGtkmmOpenGLView::OnMouseRelease(int button, int x, int y)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseRelease"<<std::endl;
    return false;
  }

  bool cGtkmmOpenGLView::OnMouseScrollUp(int x, int y)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseScrollUp"<<std::endl;
    return false;
  }

  bool cGtkmmOpenGLView::OnMouseScrollDown(int x, int y)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseScrollDown"<<std::endl;
    return false;
  }

  bool cGtkmmOpenGLView::OnMouseMove(int x, int y)
  {
    LOG<<"cGtkmmOpenGLView::OnMouseMove"<<std::endl;
    return false;
  }

  void cGtkmmOpenGLView::OnScrollBarScrolled(float fValue)
  {
    LOG<<"cGtkmmOpenGLView::OnScrollBarScrolled"<<std::endl;
    fScrollPosition = fValue;
  }
}
