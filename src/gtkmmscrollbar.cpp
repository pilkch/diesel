// Standard headers
#include <iostream>

// Gtkmm headers
#include <gtkmm/main.h>

// Spitfire headers
#include <spitfire/spitfire.h>

// Diesel headers
#include "gtkmmmainwindow.h"
#include "gtkmmscrollbar.h"

namespace diesel
{
  cGtkmmScrollBar::cGtkmmScrollBar(cGtkmmMainWindow& _parent) :
    parent(_parent),
    step(1),
    page(10)
  {
  }

  void cGtkmmScrollBar::SetStepAndPageSize(size_t _step, size_t _page)
  {
    step = _step;
    page = _page;

    const double dStep = step;
    const double dPage = page;
    set_increments(dStep, dPage);
  }

  void cGtkmmScrollBar::ScrollUp()
  {
    set_value(get_value() - step);
  }

  void cGtkmmScrollBar::ScrollDown()
  {
    set_value(get_value() + step);
  }

  void cGtkmmScrollBar::PageUp()
  {
    set_value(get_value() - page);
  }

  void cGtkmmScrollBar::PageDown()
  {
    set_value(get_value() + page);
  }

  void cGtkmmScrollBar::on_value_changed()
  {
    parent.OnScrollBarScrolled(*this);
  }
}
