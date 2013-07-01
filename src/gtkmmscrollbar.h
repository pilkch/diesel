#ifndef gtkmmscrollbar_h
#define gtkmmscrollbar_h

// Gtkmm headers
#include <gtkmm.h>

// Spitfire headers
#include <spitfire/util/updatechecker.h>

// Diesel headers
#include "diesel.h"
#include "settings.h"

namespace diesel
{
  class cGtkmmPhotoBrowser;

  class cGtkmmScrollBar : public Gtk::Scrollbar
  {
  public:
    explicit cGtkmmScrollBar(cGtkmmPhotoBrowser& parent);

    void SetRange(int64_t min, int64_t max);
    void SetStepAndPageSize(size_t step, size_t page);

    void ScrollTop();
    void ScrollBottom();
    void ScrollUp();
    void ScrollDown();
    void PageUp();
    void PageDown();

  private:
    virtual void on_value_changed() override;

    cGtkmmPhotoBrowser& parent;

    int64_t min;
    int64_t max;
    size_t step;
    size_t page;
  };
}

#endif // gtkmmscrollbar_h
