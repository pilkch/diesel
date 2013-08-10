// Standard headers
#include <iostream>

// Gtk headers
#include <gdk/gdkkeysyms-compat.h>

// Gtkmm headers
#include <gtkmm/main.h>

// libgtkmm headers
#include <libgtkmm/about.h>
#include <libgtkmm/filebrowse.h>

// Spitfire headers
#include <spitfire/spitfire.h>

// Diesel headers
#include "gtkmmmainwindow.h"
#include "gtkmmpreferencesdialog.h"

namespace diesel
{
  cGtkmmMainWindow::cGtkmmMainWindow(int argc, char** argv) :
    updateChecker(*this),
    pMenuPopup(nullptr),
    comboBoxFolder(true),
    statusBar("0 photos"),
    photoBrowser(*this)
  {
    settings.Load();

    set_title(BUILD_APPLICATION_NAME);
    set_size_request(400, 400);
    set_default_size(800, 800);
    resize(400, 400);

    // Menu and toolbar

    // void Gtk::Application::set_app_menu(const Glib::RefPtr< Gio::MenuModel > &  app_menu)
    // Sets or unsets the application menu for application.
    // The application menu is a single menu containing items that typically impact the application as a whole, rather than acting on a specific window or document. For example, you would expect to see "Preferences" or "Quit" in an application menu, but not "Save" or "Print".
    // If supported, the application menu will be rendered by the desktop environment.
    // You might call this method in your Application::property_startup() signal handler.
    // Use the base ActionMap interface to add actions, to respond to the user selecting these menu items.
    // Since gtkmm 3.4:


    // Create actions for menus and toolbars
    m_refActionGroup = Gtk::ActionGroup::create();

    // File menu
    m_refActionGroup->add(Gtk::Action::create("FileMenu", "File"));
    m_refActionGroup->add(Gtk::Action::create("FileAddFiles", "Add Files"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnMenuFileBrowseFiles));
    m_refActionGroup->add(Gtk::Action::create("FileAddFolder", "Add Folder"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnMenuFileBrowseFolder));
    m_refActionGroup->add(Gtk::Action::create("FileAddFilesFromPicturesFolder", "Add Files From Pictures Folder"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionAddFilesFromPicturesFolder));
    //m_refActionGroup->add(Gtk::Action::create("FileRemove", "Remove"),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionRemoveTrack));
    //m_refActionGroup->add(Gtk::Action::create("FileMoveToFolderMenu", "Move to Folder"));
    //m_refActionGroup->add(Gtk::Action::create("FileMoveToFolderBrowse", "Browse..."),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackMoveToFolderBrowse));
    //m_refActionGroup->add(Gtk::Action::create("FileMoveToRubbishBin", "Move to the Rubbish Bin"),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackMoveToRubbishBin));
    //m_refActionGroup->add(Gtk::Action::create("FileShowInFileManager", "Show in the File Manager"),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackShowInFileManager));
    //m_refActionGroup->add(Gtk::Action::create("FileProperties", "Properties"),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackProperties));
    m_refActionGroup->add(Gtk::Action::create("FileQuit", Gtk::Stock::QUIT),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnMenuFileQuit));

    // Edit menu
    m_refActionGroup->add(Gtk::Action::create("EditMenu", "Edit"));
    //Edit Tags -> edits each track separately
    //Batch Edit Tags -> edits all tracks at the same time
    m_refActionGroup->add(Gtk::Action::create("EditPreferences", Gtk::Stock::PREFERENCES),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnMenuEditPreferences));

    // Playback menu
    m_refActionGroup->add(Gtk::Action::create("PlaybackMenu", "Playback"));
    //m_refActionGroup->add(Gtk::Action::create("PlaybackPrevious", Gtk::Stock::MEDIA_PREVIOUS),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnPlaybackPreviousClicked));
    //pPlayPauseAction = Gtk::ToggleAction::create("PlaybackPlayPause", Gtk::Stock::MEDIA_PLAY, "Play/Pause");
    //m_refActionGroup->add(pPlayPauseAction, sigc::mem_fun(*this, &cGtkmmMainWindow::OnPlaybackPlayPauseMenuToggled));
    //m_refActionGroup->add(Gtk::Action::create("PlaybackNext", Gtk::Stock::MEDIA_NEXT),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnPlaybackNextClicked));
    //m_refActionGroup->add(Gtk::Action::create("JumpToPlaying", "Jump to Playing"),
    //        Gtk::AccelKey("<control>J"),
    //        sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionJumpToPlaying));
    //pRepeatAction = Gtk::ToggleAction::create("PlaybackRepeatToggle", Gtk::Stock::GOTO_TOP, "Repeat");
    //m_refActionGroup->add(pRepeatAction, sigc::mem_fun(*this, &cGtkmmMainWindow::OnPlaybackRepeatMenuToggled));

    // Help menu
    m_refActionGroup->add( Gtk::Action::create("HelpMenu", "Help") );
    m_refActionGroup->add( Gtk::Action::create("HelpAbout", Gtk::Stock::ABOUT),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnMenuHelpAbout) );

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);

    add_accel_group(m_refUIManager->get_accel_group());

    // Layout the actions in a menubar and toolbar
    {
      Glib::ustring ui_info =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='FileMenu'>"
        "      <menuitem action='FileAddFiles'/>"
        "      <menuitem action='FileAddFolder'/>"
        "      <menuitem action='FileAddFilesFromPicturesFolder'/>"
        //"      <menuitem action='FileRemove'/>"
        //"      <menu action='FileMoveToFolderMenu'>"
        //"        <menuitem action='FileMoveToFolderBrowse'/>"
        //"      </menu>"
        //"      <menuitem action='FileMoveToRubbishBin'/>"
        //"      <menuitem action='FileShowInFileManager'/>"
        //"      <menuitem action='FileProperties'/>"
        //"      <separator/>"
        "      <menuitem action='FileQuit'/>"
        "    </menu>"
        "    <menu action='EditMenu'>"
        "      <menuitem action='EditPreferences'/>"
        "    </menu>"
        "    <menu action='PlaybackMenu'>"
        //"      <menuitem action='PlaybackPrevious'/>"
        //"      <menuitem action='PlaybackPlayPause'/>"
        //"      <menuitem action='PlaybackNext'/>"
        //"      <menuitem action='JumpToPlaying'/>"
        //"      <separator/>"
        //"      <menuitem action='PlaybackRepeatToggle'/>"
        "    </menu>"
        "    <menu action='HelpMenu'>"
        "      <menuitem action='HelpAbout'/>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar name='ToolBar'>"
        //"    <toolitem action='PlaybackPrevious'/>"
        //"    <toolitem action='PlaybackPlayPause'/>"
        //"    <toolitem action='PlaybackNext'/>"
        //"    <toolitem action='PlaybackRepeatToggle'/>"
        "  </toolbar>"
        "</ui>";

      try {
        m_refUIManager->add_ui_from_string(ui_info);
      }
      catch(const Glib::Error& ex) {
        std::cerr<<"building menus failed: "<<ex.what();
      }
    }

    // Get the menubar and toolbar widgets, and add them to a container widget
    Gtk::Widget* pMenubar = m_refUIManager->get_widget("/MenuBar");
    if (pMenubar != nullptr) boxMainWindow.pack_start(*pMenubar, Gtk::PACK_SHRINK);


    // Photo browser right click menu
    Glib::RefPtr<Gtk::ActionGroup> popupActionGroupRef;


    // Right click menu
    popupActionGroupRef = Gtk::ActionGroup::create();

    // File|New sub menu:
    // These menu actions would normally already exist for a main menu, because a context menu should
    // not normally contain menu items that are only available via a context menu.
    popupActionGroupRef->add(Gtk::Action::create("ContextMenu", "Context Menu"));

    popupActionGroupRef->add(Gtk::Action::create("ContextAddFiles", "Add Files"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionBrowseFiles));

    popupActionGroupRef->add(Gtk::Action::create("ContextAddFolder", "Add Folder"),
            Gtk::AccelKey("<control>P"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionBrowseFolder));

    popupActionGroupRef->add(Gtk::Action::create("ContextRemove", "Remove"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionRemovePhoto));

    //Edit Tags -> edits each track separately
    //Batch Edit Tags -> edits all tracks at the same time

    /*/popupActionGroupRef->add(Gtk::Action::create("ContextMoveToFolderMenu", "Move to Folder"));

    popupActionGroupRef->add(Gtk::Action::create("ContextTrackMoveToFolderBrowse", "Browse..."),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackMoveToFolderBrowse));

    popupActionGroupRef->add(Gtk::Action::create("ContextTrackMoveToRubbishBin", "Move to the Rubbish Bin"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackMoveToRubbishBin));

    popupActionGroupRef->add(Gtk::Action::create("ContextShowInFileManager", "Show in the File Manager"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackShowInFileManager));

    popupActionGroupRef->add(Gtk::Action::create("ContextProperties", "Properties"),
            sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionTrackProperties));*/

    popupUIManagerRef = Gtk::UIManager::create();
    popupUIManagerRef->insert_action_group(popupActionGroupRef);

    add_accel_group(popupUIManagerRef->get_accel_group());

    // Layout the actions in our popup menu
    {
      Glib::ustring ui_info =
        "<ui>"
        "  <popup name='PopupMenu'>"
        "    <menuitem action='ContextAddFiles'/>"
        "    <menuitem action='ContextAddFolder'/>"
        "    <menuitem action='ContextRemove'/>"
        /*"    <menu action='ContextMoveToFolderMenu'>"
        "      <menuitem action='ContextTrackMoveToFolderBrowse'/>"
        "    </menu>"
        "    <menuitem action='ContextTrackMoveToRubbishBin'/>"
        "    <menuitem action='ContextShowInFileManager'/>"
        "    <menuitem action='ContextProperties'/>"*/
        "  </popup>"
        "</ui>";

      try
      {
        popupUIManagerRef->add_ui_from_string(ui_info);
      }
      catch(const Glib::Error& ex)
      {
        std::cerr<<"building menus failed: "<<ex.what();
      }
    }

    // Get the menu
    pMenuPopup = dynamic_cast<Gtk::Menu*>(popupUIManagerRef->get_widget("/PopupMenu"));
    if (pMenuPopup == nullptr) g_warning("Popup menu not found");
    assert(pMenuPopup != nullptr);


    // In gtkmm 3 set_orientation is not supported so we create our own toolbar out of plain old buttons
    //Gtk::Toolbar* pToolbar = dynamic_cast<Gtk::Toolbar*>(m_refUIManager->get_widget("/ToolBar"));
    //if (pToolbar != nullptr) {
    //  pToolbar->property_orientation() = Gtk::ORIENTATION_VERTICAL;
    //  //pToolbar->set_orientation(Gtk::ORIENTATION_VERTICAL);
    //  pToolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
    //  boxToolbar.pack_start(*pToolbar);
    //}

    buttonAddFiles.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionBrowseFiles));
    buttonAddFolder.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionBrowseFolder));

    boxToolbar.pack_start(buttonFolderUp, Gtk::PACK_SHRINK);
    boxToolbar.pack_start(buttonFolderShowInFileManager, Gtk::PACK_SHRINK);
    boxToolbar.pack_start(buttonAddFiles, Gtk::PACK_SHRINK);
    boxToolbar.pack_start(buttonAddFolder, Gtk::PACK_SHRINK);

    boxToolbar.pack_start(*Gtk::manage(new Gtk::Separator()), Gtk::PACK_SHRINK);


    // Controls
    comboBoxFolder.get_entry()->signal_changed().connect(sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionChangeFolder));
    buttonFolderUp.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionFolderUp));
    buttonFolderUp.set_tooltip_text("Move up");
    buttonFolderShowInFileManager.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionFolderShowInFileManager));
    buttonFolderShowInFileManager.set_tooltip_text("Show folder in file manager");

    buttonStopLoading.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmMainWindow::OnActionStopLoading));

    photoBrowser.Init(argc, argv);

    boxControls.pack_start(comboBoxFolder, Gtk::PACK_SHRINK);
    boxControls.pack_start(photoBrowser.GetWidget(), Gtk::PACK_EXPAND_WIDGET);

    boxControlsAndToolbar.pack_start(boxControls, Gtk::PACK_EXPAND_WIDGET);
    boxControlsAndToolbar.pack_start(boxToolbar, Gtk::PACK_SHRINK);

    // Hide the stop button until we start loading some files
    buttonStopLoading.hide();

    boxStatusBar.pack_start(statusBar, Gtk::PACK_SHRINK);
    boxStatusBar.pack_start(buttonStopLoading, Gtk::PACK_SHRINK);
    //... show progress bar indeterminate when we are loading any files or playlists

    boxMainWindow.pack_start(boxControlsAndToolbar, Gtk::PACK_EXPAND_WIDGET);
    boxMainWindow.pack_start(boxStatusBar, Gtk::PACK_SHRINK);

    // Add the box layout to the main window
    add(boxMainWindow);


    {
      // Get a typical selection colour for the selections on our photo browser
      Gtk::Table table;
      Glib::RefPtr<Gtk::StyleContext> pStyleContext = table.get_style_context();
      ASSERT(pStyleContext);

      const Gdk::RGBA colour = pStyleContext->get_background_color(Gtk::STATE_FLAG_SELECTED);
      const spitfire::math::cColour colourSelected(colour.get_red(), colour.get_green(), colour.get_blue(), colour.get_alpha());
      photoBrowser.SetSelectionColour(colourSelected);
    }


    // Register our icon theme and update our icons
    iconTheme.RegisterThemeChangedListener(*this);

    UpdateIcons();

    ApplySettings();

    show_all_children();

    // Start the update checker now that we have finished doing the serious work
    updateChecker.Run();
  }

  void cGtkmmMainWindow::Run()
  {
    Gtk::Main::run(*this);
  }

  void cGtkmmMainWindow::ApplySettings()
  {
    comboBoxFolder.remove_all();
    comboBoxFolder.set_active_text("");

    // Add the previous paths
    settings.GetPreviousPhotoBrowserFolders(previousFolders);
    std::list<string_t>::const_iterator iter(previousFolders.begin());
    const std::list<string_t>::const_iterator iterEnd(previousFolders.end());
    while (iter != iterEnd) {
      comboBoxFolder.append(*iter);

      iter++;
    }

    // Add the "Browse..." items
    comboBoxFolder.append("Browse...");

    const string_t sFolder = settings.GetLastPhotoBrowserFolder();

    // Set the combobox text (This will also callback and change the folder on the photo browser)
    comboBoxFolder.set_active_text(sFolder);

    // Tell the photo browser the new settings
    photoBrowser.SetCacheMaximumSizeGB(settings.GetMaximumCacheSizeGB());
  }

  void cGtkmmMainWindow::OnThemeChanged()
  {
    UpdateIcons();
  }

  void cGtkmmMainWindow::UpdateIcons()
  {
    Gtk::Image* pImageFolderUp = new Gtk::Image;
    iconTheme.LoadStockIcon(sICON_GO_UP, *pImageFolderUp);
    buttonFolderUp.set_image(*pImageFolderUp);
    Gtk::Image* pImageFileManager = new Gtk::Image("data/icons/show_in_file_browser.png");
    buttonFolderShowInFileManager.set_image(*pImageFileManager);

    /*Gtk::Image* pImageFile = new Gtk::Image;
    iconTheme.LoadStockIcon(sICON_ADD, *pImageFile);
    buttonAddFiles.set_image(*pImageFile);
    Gtk::Image* pImageDirectory = new Gtk::Image;
    iconTheme.LoadStockIcon(sICON_DIRECTORY, *pImageDirectory);
    buttonAddFolder.set_image(*pImageDirectory);*/

    Gtk::Image* pImageStop = new Gtk::Image;
    iconTheme.LoadStockIconWithSizePixels(sICON_STOP, 16, *pImageStop);
    buttonStopLoading.set_image(*pImageStop);
  }

  void cGtkmmMainWindow::OnMenuFileQuit()
  {
    // Tell the update checker thread to stop soon
    if (updateChecker.IsRunning()) updateChecker.StopThreadSoon();

    // Tell the update checker thread to stop now
    if (updateChecker.IsRunning()) updateChecker.StopThreadNow();

    // Get the previous paths
    settings.SetPreviousPhotoBrowserFolders(previousFolders);

    //hide(); //Closes the main window to stop the Gtk::Main::run().
    Gtk::Main::quit();

    settings.Save();
  }

  void cGtkmmMainWindow::OnMenuFileBrowseFiles()
  {
  }

  void cGtkmmMainWindow::OnMenuFileBrowseFolder()
  {
  }

  void cGtkmmMainWindow::OnMenuEditPreferences()
  {
    LOG<<"cGtkmmMainWindow::OnMenuEditPreferences"<<std::endl;
    cGtkmmPreferencesDialog dialog(settings, *this);
    if (dialog.Run()) {
      // Update our state from the settings
      ApplySettings();
    }
  }

  void cGtkmmMainWindow::OnMenuHelpAbout()
  {
    LOG<<"cGtkmmMainWindow::OnMenuHelpAbout"<<std::endl;
    gtkmm::cGtkmmAboutDialog about;
    about.Run(*this);
  }

  void cGtkmmMainWindow::OnActionBrowseFiles()
  {
  }

  void cGtkmmMainWindow::OnActionBrowseFolder()
  {
    // Browse for the folder
    gtkmm::cGtkmmFolderDialog dialog;
    dialog.SetType(gtkmm::cGtkmmFolderDialog::TYPE::SELECT);
    dialog.SetCaption(TEXT("Select photo folder"));
    dialog.SetDefaultFolder(settings.GetLastPhotoBrowserFolder());
    if (dialog.Run(*this)) {
      // Change the folder
      ChangeFolder(dialog.GetSelectedFolder());

      // Update the combobox text
      comboBoxFolder.set_active_text(dialog.GetSelectedFolder());
    }
  }

  void cGtkmmMainWindow::OnActionAddFilesFromPicturesFolder()
  {
  }

  void cGtkmmMainWindow::ChangeFolder(const string_t& sFolder)
  {
    LOG<<"cGtkmmMainWindow::ChangeFolder \""<<sFolder<<"\""<<std::endl;

    // Check that the folder exists
    if (!spitfire::filesystem::DirectoryExists(sFolder)) {
      LOG<<"cGtkmmMainWindow::ChangeFolder Folder doesn't exist, returning"<<std::endl;
      return;
    }

    // Check if the folder already exists
    bool bFound = false;
    std::list<string_t>::const_iterator iter(previousFolders.begin());
    const std::list<string_t>::const_iterator iterEnd(previousFolders.end());
    while (iter != iterEnd) {
      if (*iter == sFolder) {
        bFound = true;
        break;
      }

      iter++;
    }

    if (!bFound) {
      LOG<<"cGtkmmMainWindow::ChangeFolder Folder was not in the list, adding it"<<std::endl;

      // Add the folder
      previousFolders.push_front(sFolder);

      // Clamp the number of folders
      const size_t n = previousFolders.size();
      for (size_t i = 15; i < n; i++) previousFolders.pop_back();

      // Add our folder to the list of folders
      comboBoxFolder.prepend(sFolder);

      // Clamp the number of folders in the combobox too
      for (size_t i = 15; i < n; i++) comboBoxFolder.remove_text(i);
    }

    LOG<<"cGtkmmMainWindow::ChangeFolder Updating the settings"<<std::endl;
    // Update our settings
    settings.SetLastPhotoBrowserFolder(sFolder);
    if (bFound) settings.SetPreviousPhotoBrowserFolders(previousFolders);
    settings.Save();

    LOG<<"cGtkmmMainWindow::ChangeFolder Update the photo browser"<<std::endl;
    // Update the photo browser
    photoBrowser.SetFolder(sFolder);
  }

  void cGtkmmMainWindow::OnActionChangeFolder()
  {
    Glib::ustring sText = comboBoxFolder.get_entry_text();
    if (sText == "Browse...") OnActionBrowseFolder();
    else if (!sText.empty()) ChangeFolder(sText);
  }

  void cGtkmmMainWindow::OnActionFolderUp()
  {
    const string_t sFolderPath = photoBrowser.GetFolder();
    const string_t sParentFolderPath = spitfire::filesystem::GetFolder(sFolderPath);
    if (spitfire::filesystem::DirectoryExists(sParentFolderPath)) {
      // Change the folder
      ChangeFolder(sParentFolderPath);

      // Update the combobox text
      comboBoxFolder.set_active_text(sParentFolderPath);
    }
  }

  void cGtkmmMainWindow::OnActionFolderShowInFileManager()
  {
    const string_t sFolderPath = photoBrowser.GetFolder();
    spitfire::filesystem::ShowFolder(sFolderPath);
  }

  void cGtkmmMainWindow::UpdateStatusBar()
  {
    std::ostringstream o;
    const size_t nSelectedCount = photoBrowser.GetSelectedPhotoCount();
    if (nSelectedCount != 0) {
      o<<nSelectedCount;
      o<<" selected of ";
    }
    const size_t nTotal = photoBrowser.GetPhotoCount();
    o<<nTotal;
    o<<" photos and folders";
    const size_t nLoadedCount = photoBrowser.GetLoadedPhotoCount();
    const size_t nLoading = (nTotal - nLoadedCount);
    if (nLoading != 0) {
      o<<", loading ";
      o<<nLoading;
      o<<" photos";
    }
    statusBar.set_text(o.str());

    // Show the stop button if we are currently loading tracks
    if (nLoading != 0) buttonStopLoading.show();
    else buttonStopLoading.hide();
  }

  void cGtkmmMainWindow::OnPhotoBrowserChangedFolder(const string_t& sFolderPath)
  {
    // Change the folder
    ChangeFolder(sFolderPath);

    // Update the combobox text
    comboBoxFolder.set_active_text(sFolderPath);
  }

  void cGtkmmMainWindow::OnPhotoBrowserLoadedFileOrFolder()
  {
    UpdateStatusBar();
  }

  void cGtkmmMainWindow::OnPhotoBrowserFileFound()
  {
    UpdateStatusBar();
  }

  void cGtkmmMainWindow::OnPhotoBrowserLoadedFilesClear()
  {
    UpdateStatusBar();
  }

  void cGtkmmMainWindow::OnPhotoBrowserSelectionChanged()
  {
    UpdateStatusBar();
  }

  void cGtkmmMainWindow::OnActionStopLoading()
  {
    photoBrowser.StopLoading();
  }

  void cGtkmmMainWindow::OnActionRemovePhoto()
  {
  }

  void cGtkmmMainWindow::OnPhotoBrowserRightClick()
  {
    assert(pMenuPopup != nullptr);
    pMenuPopup->show_all_children();
    pMenuPopup->show_all();
    const unsigned int button = 3;
    const uint32_t time = GDK_CURRENT_TIME;
    pMenuPopup->popup(button, time);
  }

  void cGtkmmMainWindow::OnNewVersionFound(int iMajorVersion, int iMinorVersion, const string_t& sDownloadPage)
  {
    LOG<<"cGtkmmMainWindow::OnNewVersionFound "<<iMajorVersion<<"."<<iMinorVersion<<", "<<sDownloadPage<<std::endl;
  }
}
