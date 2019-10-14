// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/OptionsDialogPresenter.h"
#include <QSettings>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 */
OptionsDialogPresenter::OptionsDialogPresenter(OptionsDialog *view)
    : m_view(view){}

/* Loads the settings saved by the user */
void OptionsDialogPresenter::loadSettings(std::map<QString, QVariant>& options) {
  QSettings settings;
  const QString settingsGroupName = "Mantid/MantidWidgets/ISISReflectometryUI";
  settings.beginGroup(settingsGroupName);
  QStringList keys = settings.childKeys();
  for (auto &key : keys)
    options[key] = settings.value(key);
  settings.endGroup();
}

/* Saves the settings specified by the user */
void OptionsDialogPresenter::saveSettings(
    const std::map<QString, QVariant> &options) {
  QSettings settings;
  const QString settingsGroupName = "Mantid/MantidWidgets/ISISReflectometryUI";
  settings.beginGroup(settingsGroupName);
  QStringList keys = settings.childKeys();
  for (const auto &option : options)
    settings.setValue(option.first, option.second);
  settings.endGroup();
}

/** Load options from disk if possible, or set to defaults */
void OptionsDialogPresenter::initOptions() {
  m_options.clear();
  applyDefaultOptions(m_options);

  // TODO: Apply checks

  // Load saved values from disk
  loadSettings(m_options);
}

void OptionsDialogPresenter::applyDefaultOptions(
    std::map<QString, QVariant> &options) {
  options["WarnProcessAll"] = true;
  options["WarnDiscardChanges"] = true;
  options["WarnProcessPartialGroup"] = true;
  options["Round"] = false;
  options["RoundPrecision"] = 3;
}

/** Loads the options used into the view */
void
OptionsDialogPresenter::loadOptions() {
  loadSettings(m_options);
  m_view->setOptions(m_options);
}

/** Saves the options selected in the view */
void OptionsDialogPresenter::saveOptions() {
  m_view->getOptions(m_options);
  saveSettings(m_options);  
}

} // namespace MantidWidgets
} // namespace MantidQt