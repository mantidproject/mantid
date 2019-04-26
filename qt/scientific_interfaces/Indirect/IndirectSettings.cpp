// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettings.h"

namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectSettings)

IndirectSettings::IndirectSettings(QWidget *parent)
    : MantidQt::API::UserSubWindow(parent) {
  m_uiForm.setupUi(this);
}

void IndirectSettings::initLayout() {
  m_presenter = Mantid::Kernel::make_unique<IndirectSettingsPresenter>(this);

  auto centralWidget = m_uiForm.centralWidget->layout();
  centralWidget->addWidget(m_presenter->getView());

  connect(m_presenter.get(), SIGNAL(applySettings()), this,
          SIGNAL(applySettings()));
  connect(m_presenter.get(), SIGNAL(closeSettings()), this,
          SLOT(closeSettings()));
}

std::map<std::string, QVariant> IndirectSettings::getSettings() const {
  std::map<std::string, QVariant> settings;
  settings["RestrictInput"] = m_presenter->getSetting("restrict-input-by-name");
  settings["ErrorBars"] = m_presenter->getSetting("plot-error-bars");
  return settings;
}

void IndirectSettings::loadSettings() { m_presenter->loadSettings(); }

void IndirectSettings::closeSettings() {
  if (window())
    window()->close();
}

} // namespace CustomInterfaces
} // namespace MantidQt
