#ifndef gtkmmphotobrowser_h
#define gtkmmphotobrowser_h

// Gtkmm headers
#include <gtkmm.h>

// Diesel headers
#include "diesel.h"
#include "gtkmmopenglview.h"
#include "gtkmmscrollbar.h"

namespace diesel
{
  class cGtkmmMainWindow;

  class cGtkmmPhotoBrowser
  {
  public:
    friend class cGtkmmOpenGLView;
    friend class cGtkmmScrollBar;

    explicit cGtkmmPhotoBrowser(cGtkmmMainWindow& parent);

    void Init(int argc, char** argv);

    const Gtk::Widget& GetWidget() const;
    Gtk::Widget& GetWidget();

    void SetSelectionColour(const spitfire::math::cColour& colour);

    bool IsOpenGLViewFocus() const;

    string_t GetFolder() const;
    void SetFolder(const string_t& sFolderPath);

    size_t GetPhotoCount() const;
    size_t GetLoadedPhotoCount() const;
    size_t GetSelectedPhotoCount() const;

    void StopLoading();

  protected:
    // Events generated by the scrollbar
    void OnScrollBarScrolled(const cGtkmmScrollBar& widget);

    // Events generated by the OpenGL view
    void OnOpenGLViewLoadedFileOrFolder();
    void OnOpenGLViewFileFound();
    void OnOpenGLViewLoadedFilesClear();
    void OnOpenGLViewSelectionChanged();
    void OnOpenGLViewContentChanged();
    void OnOpenGLViewRightClick();
    void OnOpenGLViewScrollTop();
    void OnOpenGLViewScrollBottom();
    void OnOpenGLViewScrollPageUp();
    void OnOpenGLViewScrollPageDown();

  private:
    bool event_box_key_press(GdkEventKey* pEvent);
    bool event_box_button_press(GdkEventButton* pEvent);
    bool event_box_button_release(GdkEventButton* pEvent);
    bool event_box_scroll(GdkEventScroll* pEvent);
    bool event_box_motion_notify(GdkEventMotion* pEvent);

    cGtkmmMainWindow& parent;

    cGtkmmOpenGLView openglView;

    cGtkmmScrollBar scrollBar;

    Gtk::EventBox eventBoxOpenglView;

    Gtk::HBox boxPhotoView;
  };
}

#endif // gtkmmphotobrowser_h
