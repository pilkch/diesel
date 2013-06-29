#ifndef gtkmmscrollbar_h
#define gtkmmscrollbar_h

// Gtkmm headers
#include <gtkmm.h>

// Spitfire headers
#include <spitfire/util/updatechecker.h>

// Diesel headers
#include "diesel.h"
#include "gtkmmopenglview.h"
#include "gtkmmscrollbar.h"
#include "settings.h"

namespace diesel
{
  class cGtkmmMainWindow;

  class cGtkmmScrollBar : public Gtk::Scrollbar
  {
  public:
    explicit cGtkmmScrollBar(cGtkmmMainWindow& parent);

    void SetStepAndPageSize(size_t step, size_t page);

    void ScrollUp();
    void ScrollDown();
    void PageUp();
    void PageDown();

  private:
    virtual void on_value_changed() override;

    cGtkmmMainWindow& parent;

    size_t step;
    size_t page;
  };
}

#endif // gtkmmscrollbar_h
