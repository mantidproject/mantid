// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "OptionsDialogPresenter.h"
#include "IOptionsDialogView.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 * @param model :: a handle to a model for this presenter
 */
OptionsDialogPresenter::OptionsDialogPresenter(IOptionsDialogView *view, std::unique_ptr<IOptionsDialogModel> model)
    : m_view(view), m_model(std::move(model)), m_notifyee() {
  initOptions();
  m_view->subscribe(this);
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
    m_model->applyDefaultOptions(m_boolOptions, m_intOptions);
}
/** Loads the options used into the view */
void OptionsDialogPresenter::notifyLoadOptions() {
  m_model->loadSettings(m_boolOptions, m_intOptions);
  m_view->setOptions(m_boolOptions, m_intOptions);
  m_notifyee->notifyOptionsChanged();
}

/** Saves the options selected in the view */
void OptionsDialogPresenter::notifySaveOptions() {
  m_view->getOptions(m_boolOptions, m_intOptions);
  m_model->saveSettings(m_boolOptions, m_intOptions);
  m_notifyee->notifyOptionsChanged();
}

/* Get a bool option state */
bool OptionsDialogPresenter::getBoolOption(const std::string &optionName) { return m_boolOptions[optionName]; }

/* Get an int option state */
int &OptionsDialogPresenter::getIntOption(const std::string &optionName) { return m_intOptions[optionName]; }

void OptionsDialogPresenter::showView() { m_view->show(); }

void OptionsDialogPresenter::subscribe(OptionsDialogPresenterSubscriber *notifyee) {
  m_notifyee = notifyee;
  // the following call is required after m_notifyee is set, rather than
  // in the constructor, in order to avoid a segfault since
  // notifyLoadOptions() makes a call to m_notifyee->notifyOptionsChanged() but
  // m_notifyee would be nullptr
  notifyLoadOptions();
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
