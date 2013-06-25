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
    scale(1.0f, 1.0f),
    pContext(nullptr),
    pShaderPhoto(nullptr),
    pStaticVertexBufferObjectPhoto(nullptr)
  {
    // Set our resolution
    resolution.width = 500;
    resolution.height = 500;
    resolution.pixelFormat = opengl::PIXELFORMAT::R8G8B8A8;

    set_size_request(resolution.width, resolution.height);

    // Allow this control to grab the keyboard focus
    set_can_focus(true);
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
      }
    }

    // Create our vertex buffer object
    pStaticVertexBufferObjectPhoto = pContext->CreateStaticVertexBufferObject();
    ASSERT(pStaticVertexBufferObjectPhoto != nullptr);
    CreateVertexBufferObjectPhoto(pStaticVertexBufferObjectPhoto, 3040, 2024);

    // Create our shader
    pShaderPhoto = pContext->CreateShader(TEXT("data/shaders/passthrough.vert"), TEXT("data/shaders/passthrough.frag"));
    ASSERT(pShaderPhoto != nullptr);
  }

  void cGtkmmOpenGLView::DestroyResources()
  {
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

    const spitfire::math::cMat4 matProjection = pContext->CalculateProjectionMatrix();

    const spitfire::math::cMat4 matView;

    const float fSpacingX = 0.1f;
    const float fSpacingY = 0.1f;

    spitfire::math::cMat4 matTranslationPhoto[9];
    size_t i = 0;
    for (size_t y = 0; y < 3; y++) {
      for (size_t x = 0; x < 3; x++) {
        const spitfire::math::cVec3 position((-1.0f + float(x)) * fSpacingX, ((-2.0f - float(y)) * fSpacingY), 0.0f);
        matTranslationPhoto[i].SetTranslation(position);
        i++;
      }
    }

    bool bIsRotating = true;
    float fAngleRadians = 0.0f;
    const spitfire::sampletime_t uiUpdateDelta = 10;
    const float fRotationSpeed = 0.1f;

    spitfire::math::cMat4 matObjectRotation;

    {
      // Update object rotation
      if (bIsRotating) fAngleRadians += float(uiUpdateDelta) * fRotationSpeed;

      spitfire::math::cQuaternion rotation;
      rotation.SetFromAxisAngle(spitfire::math::v3Up, fAngleRadians);

      matObjectRotation.SetRotation(rotation);
    }

    pContext->SetClearColour(spitfire::math::cColour(0.0f, 0.0f, 0.0f, 1.0f));

    pContext->BeginRenderToScreen();

    if (bIsWireframe) pContext->EnableWireframe();

    // Render the photos
    {
      pContext->BeginRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO);

      spitfire::math::cMat4 matModelView2D;

      spitfire::math::cMat4 matScale;
      matScale.SetScale(scale.x, scale.y, 1.0f);

      pContext->BindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);

      const float fWidth = resolution.width - (fThumbNailSpacing + fThumbNailSpacing);

      const size_t columns = fWidth / (fThumbNailWidth + fThumbNailSpacing);

      const size_t nPhotos = photoTextures.size();
      for (size_t i = 0; i < nPhotos; i++) {
        opengl::cTexture* pTexture = photoTextures[i];

        pContext->BindTexture(0, *pTexture);

        pContext->BindShader(*pShaderPhoto);

        const size_t x = i % columns;
        const size_t y = i / columns;
        matModelView2D.SetTranslation(fThumbNailSpacing + (float(x) * (fThumbNailWidth + fThumbNailSpacing)), fThumbNailSpacing + (float(y) * (fThumbNailHeight + fThumbNailSpacing)), 0.0f);

        pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matScale * matModelView2D);

        pContext->DrawStaticVertexBufferObjectTriangles2D(*pStaticVertexBufferObjectPhoto);

        pContext->UnBindShader(*pShaderPhoto);

        pContext->UnBindTexture(0, *pTexture);
      }

      pContext->UnBindStaticVertexBufferObject2D(*pStaticVertexBufferObjectPhoto);

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
}
