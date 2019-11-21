// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/OptionsDialogPresenter.h"
#include "MantidQtWidgets/Common/IOptionsDialog.h"
#include "MantidQtWidgets/Common/QSettingsHelper.h"
#include <QSettings>

namespace MantidQt {
namespace MantidWidgets {

using namespace MantidQt::MantidWidgets::QSettingsHelper;

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 * @param model :: a handle to a model for this presenter
 */
OptionsDialogPresenter::OptionsDialogPresenter(IOptionsDialog *view)
    : m_view(view) {
  m_view->subscribe(this);
}

/* Loads the settings saved by the user */
void OptionsDialogPresenter::loadSettings(
    std::map<std::string, bool> &boolOptions,
    std::map<std::string, int> &intOptions) {
  boolOptions = getSettingsAsMap<bool>(REFLECTOMETRY_SETTINGS_GROUP);
  intOptions = getSettingsAsMap<int>(REFLECTOMETRY_SETTINGS_GROUP);
}

/* Saves the settings specified by the user */
void OptionsDialogPresenter::saveSettings(
    const std::map<std::string, bool> &boolOptions, const std::map<std::string, int> &intOptions) {
  for (const auto &boolOption : boolOptions)
    setSetting(REFLECTOMETRY_SETTINGS_GROUP, boolOption.first, boolOption.second);
  for (const auto &intOption : intOptions)
    setSetting(REFLECTOMETRY_SETTINGS_GROUP, intOption.first,
               intOption.second);
}

/** Load options from disk if possible, or set to defaults */
void OptionsDialogPresenter::initOptions() {
  m_boolOptions.clear();
  m_intOptions.clear();
  applyDefaultOptions(m_boolOptions, m_intOptions);

  // TODO: Apply checks

  // Load saved values from disk
  loadSettings(m_boolOptions, m_intOptions);
}

void OptionsDialogPresenter::applyDefaultOptions(
    std::map<std::string, bool> &boolOptions,
    std::map<std::string, int> &intOptions) {
  boolOptions["WarnProcessAll"] = true;
  boolOptions["WarnDiscardChanges"] = true;
  boolOptions["WarnProcessPartialGroup"] = true;
  boolOptions["Round"] = false;
  intOptions["RoundPrecision"] = 3;
}

/** Loads the options used into the view */
void
OptionsDialogPresenter::loadOptions() {
  loadSettings(m_boolOptions, m_intOptions);
  m_view->setOptions(m_boolOptions, m_intOptions);
}

/** Saves the options selected in the view */
void OptionsDialogPresenter::saveOptions() {
  m_view->getOptions(m_boolOptions, m_intOptions);

  // TODO Update model

  saveSettings(m_boolOptions, m_intOptions);  
}

void OptionsDialogPresenter::showView() { m_view->show(); }

} // namespace MantidWidgets
} // namespace MantidQt