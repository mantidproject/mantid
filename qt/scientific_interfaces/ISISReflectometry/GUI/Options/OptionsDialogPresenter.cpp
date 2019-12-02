// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "OptionsDialogPresenter.h"
#include "IOptionsDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 * @param model :: a handle to a model for this presenter
 */
OptionsDialogPresenter::OptionsDialogPresenter(IOptionsDialog *view)
    : m_view(view), m_mainPresenter(), m_model(OptionsDialogModel()) {
  initOptions();
  m_view->subscribe(this);
}

/** Accept a main presenter
 * @param mainPresenter :: [input] A main presenter
 */
void OptionsDialogPresenter::acceptMainPresenter(
    IMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

/** Load options from disk if possible, or set to defaults */
void OptionsDialogPresenter::initOptions() {
  m_boolOptions.clear();
  m_intOptions.clear();
  // Attempt to load saved values from disk
  m_model.loadSettings(m_boolOptions, m_intOptions);
  // If unsuccessful, load defaults
  if (m_boolOptions.empty() || m_intOptions.empty())
    notifyApplyDefaultOptions(m_boolOptions, m_intOptions);
}

void OptionsDialogPresenter::notifyApplyDefaultOptions(
    std::map<std::string, bool> &boolOptions,
    std::map<std::string, int> &intOptions) {
  m_model.applyDefaultOptions(boolOptions, intOptions);
}

/** Loads the options used into the view */
void OptionsDialogPresenter::loadOptions() {
  m_model.loadSettings(m_boolOptions, m_intOptions);
  m_view->setOptions(m_boolOptions, m_intOptions);
  m_mainPresenter->notifyOptionsChanged();
}

/** Saves the options selected in the view */
void OptionsDialogPresenter::saveOptions() {
  m_view->getOptions(m_boolOptions, m_intOptions);
  m_model.saveSettings(m_boolOptions, m_intOptions);
  m_mainPresenter->notifyOptionsChanged();
}

/* Get a bool option state */
bool OptionsDialogPresenter::getBoolOption(std::string &optionName) {
  return m_boolOptions[optionName];
}

/* Get an int option state */
int OptionsDialogPresenter::getIntOption(std::string &optionName) {
  return m_intOptions[optionName];
}

void OptionsDialogPresenter::showView() { m_view->show(); }

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt