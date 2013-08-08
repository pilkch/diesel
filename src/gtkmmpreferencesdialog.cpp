// Spitfire headers
#include <spitfire/communication/network.h>

// Diesel headers
#include "gtkmmpreferencesdialog.h"
#include "imagecachemanager.h"

namespace diesel
{
  cGtkmmPreferencesDialog::cGtkmmPreferencesDialog(cSettings& _settings, Gtk::Window& parent) :
    Gtk::Dialog("Preferences", parent, true),
    settings(_settings),
    groupHistory("History"),
    historyClearFolders("Clear previous folders"),
    historyClearCache("Clear cache"),
    pOkButton(nullptr),
    nCacheMaximumSizeGB(settings.GetMaximumCacheSizeGB())
  {
    set_border_width(5);

    set_resizable();

    signal_response().connect(sigc::mem_fun(*this, &cGtkmmPreferencesDialog::OnResponse));

    Gtk::Box* pBox = get_vbox();

    pBox->set_border_width(10);

    pBox->pack_start(groupHistory, Gtk::PACK_SHRINK);
    groupHistory.add(boxHistory);
    boxHistory.pack_start(historyClearFolders, Gtk::PACK_SHRINK);
    historyClearFolders.set_border_width(10);

    Gtk::Label* pCacheSize = Gtk::manage(new Gtk::Label("Cache Size:"));
    boxCacheMaximumSize.pack_start(*pCacheSize, Gtk::PACK_SHRINK);
    historyCacheMaximumSizeGB.set_range(1.0, 20.0);
    historyCacheMaximumSizeGB.set_increments(1.0, 5.0);
    historyCacheMaximumSizeGB.set_value(nCacheMaximumSizeGB);
    boxCacheMaximumSize.pack_start(historyCacheMaximumSizeGB, Gtk::PACK_SHRINK);
    Gtk::Label* pGB = Gtk::manage(new Gtk::Label("GB"));
    boxCacheMaximumSize.pack_start(*pGB, Gtk::PACK_SHRINK);
    boxCacheMaximumSize.set_border_width(10);
    boxHistory.pack_start(boxCacheMaximumSize, Gtk::PACK_SHRINK);

    boxHistory.pack_start(historyClearCache, Gtk::PACK_SHRINK);
    historyClearCache.set_border_width(10);

    historyClearFolders.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmPreferencesDialog::OnActionHistoryClearFolders));
    historyCacheMaximumSizeGB.signal_changed().connect(sigc::mem_fun(*this, &cGtkmmPreferencesDialog::OnActionHistoryCacheMaximumSizeGBChanged));
    historyClearCache.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmPreferencesDialog::OnActionHistoryClearCache));

    // Add separator
    pBox->pack_start(separator, Gtk::PACK_SHRINK);

    // Add standard buttons
    add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
    pOkButton = add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);

    // Make sure that our controls are in the correct state
    OnEnableControls();

    show_all_children();
  }

  void cGtkmmPreferencesDialog::OnResponse(int response_id)
  {
    if (response_id == Gtk::ResponseType::RESPONSE_OK) {
      /*settings.SetRepeat(playbackRepeat.get_active());
      settings.SetNotifyOnSongChange(playbackNotifyOnSongChange.get_active());
      settings.SetNextSongOnMoveToTrash(playbackNextSongOnMoveToTrash.get_active());*/

      if (settings.GetMaximumCacheSizeGB() != nCacheMaximumSizeGB) settings.SetMaximumCacheSizeGB(nCacheMaximumSizeGB);

      settings.Save();
    }
  }

  void cGtkmmPreferencesDialog::OnActionHistoryClearFolders()
  {
    std::list<string_t> folders;
    settings.SetPreviousPhotoBrowserFolders(folders);
    settings.SetLastPhotoBrowserFolder("");
  }

  void cGtkmmPreferencesDialog::OnActionHistoryCacheMaximumSizeGBChanged()
  {
    nCacheMaximumSizeGB = historyCacheMaximumSizeGB.get_value();
  }

  void cGtkmmPreferencesDialog::OnActionHistoryClearCache()
  {
    cImageCacheManager::ClearCache();
  }

  void cGtkmmPreferencesDialog::OnEnableControls()
  {
    /*bool bEnabled = lastfmEnabled.get_active();
    lastfmUserNameDescription.set_sensitive(bEnabled);
    lastfmUserName.set_sensitive(bEnabled);
    lastfmPasswordDescription.set_sensitive(bEnabled);
    lastfmPassword.set_sensitive(bEnabled);

    if (pOkButton != nullptr) pOkButton->set_sensitive(!bEnabled || (bEnabled && !lastfmUserName.get_text().empty() && !lastfmPassword.get_text().empty()));

    bEnabled = webServerEnabled.get_active();
    const size_t n = webServerLinks.size();
    for (size_t i = 0; i < n; i++) webServerLinks[i]->set_sensitive(bEnabled);*/
  }

  bool cGtkmmPreferencesDialog::Run()
  {
    return (run() == Gtk::ResponseType::RESPONSE_OK);
  }
}
