// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "OptionsDialogPresenter.h"
#include "IOptionsDialogView.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 * @param model :: a handle to a model for this presenter
 */
OptionsDialogPresenter::OptionsDialogPresenter(
    IOptionsDialogView *view, std::unique_ptr<IOptionsDialogModel> model)
    : m_view(view), m_model(std::move(model)), m_mainWindowNotifyee() {}

/** Allow options to be initialised once the main presenter
    is constructed (necessary due to rounding impl)
 */
void OptionsDialogPresenter::notifyInitOptions() { initOptions(); }

void OptionsDialogPresenter::notifyOptionsChanged() {
  m_mainWindowNotifyee->optionsChanged();
}

/** Subscribe the view to this presenter */
void OptionsDialogPresenter::notifySubscribeView() { m_view->subscribe(this); }

/** Load options from disk if possible, or set to defaults */
void OptionsDialogPresenter::initOptions() {
  m_boolOptions.clear();
  m_intOptions.clear();
  // Attempt to load saved values from disk
  m_model->loadSettings(m_boolOptions, m_intOptions);
  // If unsuccessful, load defaults
  if (m_boolOptions.empty() || m_intOptions.empty())
    notifyApplyDefaultOptions(m_boolOptions, m_intOptions);
}

void OptionsDialogPresenter::notifyApplyDefaultOptions(
    std::map<std::string, bool> &boolOptions,
    std::map<std::string, int> &intOptions) {
  m_model->applyDefaultOptions(boolOptions, intOptions);
}

/** Loads the options used into the view */
void OptionsDialogPresenter::loadOptions() {
  m_model->loadSettings(m_boolOptions, m_intOptions);
  m_view->setOptions(m_boolOptions, m_intOptions);
  notifyOptionsChanged();
}

/** Saves the options selected in the view */
void OptionsDialogPresenter::saveOptions() {
  m_view->getOptions(m_boolOptions, m_intOptions);
  m_model->saveSettings(m_boolOptions, m_intOptions);
  notifyOptionsChanged();
}

/* Get a bool option state */
bool OptionsDialogPresenter::getBoolOption(std::string &optionName) {
  return m_boolOptions[optionName];
}

/* Get an int option state */
int &OptionsDialogPresenter::getIntOption(std::string &optionName) {
  return m_intOptions[optionName];
}

void OptionsDialogPresenter::showView() { m_view->show(); }

void OptionsDialogPresenter::subscribe(
    OptionsDialogMainWindowSubscriber *notifyee) {
  m_mainWindowNotifyee = notifyee;
  loadOptions();
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt