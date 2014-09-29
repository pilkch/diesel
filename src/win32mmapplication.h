#ifndef DIESEL_APPLICATION_H
#define DIESEL_APPLICATION_H

// Standard headers
#include <stack>

// libopenglmm headers
#include <libopenglmm/cFont.h>
#include <libopenglmm/cWindow.h>
#include <libopenglmm/cSystem.h>

// Spitfire headers
#include <spitfire/math/math.h>
#include <spitfire/util/cConsoleApplication.h>

#undef interface

// libwin32mm headers
#include <libwin32mm/controls.h>
#include <libwin32mm/keys.h>
#include <libwin32mm/maindialog.h>
#include <libwin32mm/openglcontrol.h>
#include <libwin32mm/taskbar.h>
#include <libwin32mm/theme.h>

#undef interface
#define interface Interface

// Diesel headers
#include "photobrowserviewcontroller.h"
#include "settings.h"
#include "win32mmopenglview.h"

namespace diesel
{
  class cWin32mmOpenGLView;
  class cApplication;

  class cMainWindow : public win32mm::cMainDialog, public cWin32mmOpenGLViewListener
  {
  public:
    friend class cWin32mmOpenGLView;

    explicit cMainWindow(cApplication& application);

  private:
    virtual void OnInit() override;
    virtual void OnInitFinished() override;
    virtual void OnDestroy() override;
    virtual bool OnQuit() override;

    virtual bool OnOk() override;
    virtual bool OnCancel() override;

    virtual void OnResizing(size_t width, size_t height) override;
    virtual void OnResize(size_t width, size_t height) override;

    virtual bool OnCommand(int idCommand) override;

    virtual void OnOpenGLViewChangedFolder(const string_t& sFolderPath) override;
    virtual void OnOpenGLViewRightClick() override;

    void LoadWindowPosition();
    void SaveWindowPosition();

  private:
    void AddMenu();
    void AddStatusBar();

    void UpdateScrollBar();

    void ResizeStatusBar();

    cApplication& application;
    cSettings& settings;

    win32mm::cTheme theme;

    win32mm::cStatusBar statusBar;
    win32mm::cTaskBar taskBar;

    cWin32mmOpenGLView photoBrowserView;
    cPhotoBrowserViewController photoBrowserViewController;

    win32mm::cComboBox comboBoxPath;
    win32mm::cButton buttonPathUp;
    win32mm::cButton buttonPathShowFolder;
    win32mm::cScrollBar scrollBar;

    bool bShouldQuit; // A slight hack so that we can tell when the Quit menu item is selected rather than the user prcessing the escape key
  };


  class cState;

  // ** cApplication

  class cApplication : public spitfire::cConsoleApplication
  {
  public:
    friend class cMainWindow;
    friend class cState;

    cApplication(int argc, const char* const* argv);
    ~cApplication();

  private:
    virtual void _PrintHelp() const override;
    virtual string_t _GetVersion() const override;
    virtual bool _Run() override;

    cSettings settings;

    cMainWindow mainWindow;
  };
}

#endif // DIESEL_APPLICATION_H
