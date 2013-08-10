// Standard headers
#include <iostream>

// Gtk headers
#include <gdk/gdkkeysyms-compat.h>

// Gtkmm headers
#include <gtkmm/main.h>

// Spitfire headers
#include <spitfire/spitfire.h>

// Diesel headers
#include "gtkmmphotobrowser.h"
#include "gtkmmmainwindow.h"

namespace diesel
{
  cGtkmmPhotoBrowser::cGtkmmPhotoBrowser(cGtkmmMainWindow& _parent) :
    parent(_parent),
    openglView(*this),
    scrollBar(*this)
  {
  }

  void cGtkmmPhotoBrowser::Init(int argc, char** argv)
  {
    // OpenGL view event box
    eventBoxOpenglView.add_events(Gdk::BUTTON_MOTION_MASK);
    eventBoxOpenglView.add_events(Gdk::SCROLL_MASK);
    eventBoxOpenglView.add_events(Gdk::POINTER_MOTION_MASK);

    eventBoxOpenglView.signal_key_press_event().connect(sigc::mem_fun(*this, &cGtkmmPhotoBrowser::event_box_key_press));
    eventBoxOpenglView.signal_button_press_event().connect(sigc::mem_fun(*this, &cGtkmmPhotoBrowser::event_box_button_press));
    eventBoxOpenglView.signal_button_release_event().connect(sigc::mem_fun(*this, &cGtkmmPhotoBrowser::event_box_button_release));
    eventBoxOpenglView.signal_scroll_event().connect(sigc::mem_fun(*this, &cGtkmmPhotoBrowser::event_box_scroll));
    eventBoxOpenglView.signal_motion_notify_event().connect(sigc::mem_fun(*this, &cGtkmmPhotoBrowser::event_box_motion_notify));

    eventBoxOpenglView.add(openglView);

    // Add the OpenGL view and scroll bar
    boxPhotoView.pack_start(eventBoxOpenglView, Gtk::PACK_EXPAND_WIDGET);
    boxPhotoView.pack_start(scrollBar, Gtk::PACK_SHRINK);

    openglView.Init(argc,  argv);


    // Set up the scroll bar
    scrollBar.set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);

    OnOpenGLViewContentChanged();
  }

  const Gtk::Widget& cGtkmmPhotoBrowser::GetWidget() const
  {
    return boxPhotoView;
  }

  Gtk::Widget& cGtkmmPhotoBrowser::GetWidget()
  {
    return boxPhotoView;
  }

  void cGtkmmPhotoBrowser::SetSelectionColour(const spitfire::math::cColour& colour)
  {
    openglView.SetSelectionColour(colour);
  }

  string_t cGtkmmPhotoBrowser::GetFolder() const
  {
    return openglView.GetFolder();
  }

  void cGtkmmPhotoBrowser::SetFolder(const string_t& sFolderPath)
  {
    openglView.SetFolder(sFolderPath);
  }

  void cGtkmmPhotoBrowser::SetCacheMaximumSizeGB(size_t nCacheMaximumSizeGB)
  {
    openglView.SetCacheMaximumSizeGB(nCacheMaximumSizeGB);
  }

  size_t cGtkmmPhotoBrowser::GetPhotoCount() const
  {
    return openglView.GetPhotoCount();
  }

  size_t cGtkmmPhotoBrowser::GetLoadedPhotoCount() const
  {
    return openglView.GetLoadedPhotoCount();
  }

  size_t cGtkmmPhotoBrowser::GetSelectedPhotoCount() const
  {
    return openglView.GetSelectedPhotoCount();
  }

  void cGtkmmPhotoBrowser::StopLoading()
  {
    openglView.StopLoading();
  }

  bool cGtkmmPhotoBrowser::event_box_key_press(GdkEventKey* pEvent)
  {
    ASSERT(pEvent != nullptr);
    return openglView.OnKeyPressEvent(pEvent);
  }

  bool cGtkmmPhotoBrowser::event_box_button_press(GdkEventButton* pEvent)
  {
    ASSERT(pEvent != nullptr);
    const bool bKeyControl = ((pEvent->state & GDK_CONTROL_MASK) != 0);
    const bool bKeyShift = ((pEvent->state & GDK_SHIFT_MASK) != 0);
    if (pEvent->type == GDK_2BUTTON_PRESS) return openglView.OnMouseDoubleClick(pEvent->button, pEvent->x, pEvent->y, bKeyControl, bKeyShift);

    return openglView.OnMouseDown(pEvent->button, pEvent->x, pEvent->y, bKeyControl, bKeyShift);
  }

  bool cGtkmmPhotoBrowser::event_box_button_release(GdkEventButton* pEvent)
  {
    ASSERT(pEvent != nullptr);
    const bool bKeyControl = ((pEvent->state & GDK_CONTROL_MASK) != 0);
    const bool bKeyShift = ((pEvent->state & GDK_SHIFT_MASK) != 0);
    return openglView.OnMouseRelease(pEvent->button, pEvent->x, pEvent->y, bKeyControl, bKeyShift);
  }

  bool cGtkmmPhotoBrowser::event_box_scroll(GdkEventScroll* pEvent)
  {
    ASSERT(pEvent != nullptr);

    if ((pEvent->state & GDK_CONTROL_MASK) != 0) {
      // Zooming
      if (pEvent->direction == GDK_SCROLL_UP) {
        openglView.SetScale(min(20.0f, openglView.GetScale() + 0.1f));
        OnOpenGLViewContentChanged();
        return true;
      } else if (pEvent->direction == GDK_SCROLL_DOWN) {
        openglView.SetScale(max(1.0f, openglView.GetScale() - 0.1f));
        OnOpenGLViewContentChanged();
        return true;
      }
    } else {
      // Scrolling
      if (pEvent->direction == GDK_SCROLL_UP) {
        scrollBar.ScrollUp();
        return true;
      } else if (pEvent->direction == GDK_SCROLL_DOWN) {
        scrollBar.ScrollDown();
        return true;
      }
    }

    return false;
  }

  bool cGtkmmPhotoBrowser::event_box_motion_notify(GdkEventMotion* pEvent)
  {
    ASSERT(pEvent != nullptr);
    const bool bKeyControl = ((pEvent->state & GDK_CONTROL_MASK) != 0);
    const bool bKeyShift = ((pEvent->state & GDK_SHIFT_MASK) != 0);
    return openglView.OnMouseMove(pEvent->x, pEvent->y, bKeyControl, bKeyShift);
  }

  void cGtkmmPhotoBrowser::OnScrollBarScrolled(const cGtkmmScrollBar& widget)
  {
    float fValue = static_cast<float>(scrollBar.get_value());
    openglView.OnScrollBarScrolled(fValue);
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewContentChanged()
  {
    scrollBar.SetRange(0, openglView.GetRequiredHeight());

    const size_t page = openglView.GetPageHeight();
    const size_t step = page / 5;
    scrollBar.SetStepAndPageSize(step, page);
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewRightClick()
  {
    parent.OnPhotoBrowserRightClick();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewScrollTop()
  {
    scrollBar.ScrollTop();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewScrollBottom()
  {
    scrollBar.ScrollBottom();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewScrollPageUp()
  {
    scrollBar.PageUp();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewScrollPageDown()
  {
    scrollBar.PageDown();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewChangedFolder(const string_t& sFolderPath)
  {
    parent.OnPhotoBrowserChangedFolder(sFolderPath);
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewLoadedFileOrFolder()
  {
    parent.OnPhotoBrowserLoadedFileOrFolder();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewFileFound()
  {
    parent.OnPhotoBrowserFileFound();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewLoadedFilesClear()
  {
    parent.OnPhotoBrowserLoadedFilesClear();
  }

  void cGtkmmPhotoBrowser::OnOpenGLViewSelectionChanged()
  {
    parent.OnPhotoBrowserSelectionChanged();
  }
}
