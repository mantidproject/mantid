// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettings.h"
#include "IndirectInterface.h"
#include "IndirectSettingsHelper.h"
#include "MantidQtIcons/Icon.h"

constexpr auto SETTINGS_ICON = "mdi.settings";

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(IndirectSettings)

IndirectSettings::IndirectSettings(QWidget *parent) : MantidQt::API::UserSubWindow(parent) { m_uiForm.setupUi(this); }

QIcon IndirectSettings::icon() { return Icons::getIcon(SETTINGS_ICON); }

void IndirectSettings::initLayout() {
  m_presenter = std::make_unique<IndirectSettingsPresenter>(this);

  auto centralWidget = m_uiForm.centralWidget->layout();
  centralWidget->addWidget(m_presenter->getView());

  connect(m_presenter.get(), SIGNAL(applySettings()), this, SIGNAL(applySettings()));
  connect(m_presenter.get(), SIGNAL(closeSettings()), this, SLOT(closeSettings()));
}

void IndirectSettings::otherUserSubWindowCreated(QPointer<UserSubWindow> window) { connectIndirectInterface(window); }

void IndirectSettings::otherUserSubWindowCreated(QList<QPointer<UserSubWindow>> &windows) {
  for (auto &window : windows)
    connectIndirectInterface(window);
}

void IndirectSettings::connectIndirectInterface(const QPointer<UserSubWindow> &window) {
  if (auto indirectInterface = dynamic_cast<IndirectInterface *>(window.data()))
    connect(m_presenter.get(), SIGNAL(applySettings()), indirectInterface, SLOT(applySettings()));
}

std::map<std::string, QVariant> IndirectSettings::getSettings() const {
  std::map<std::string, QVariant> settings;
  settings["RestrictInput"] = IndirectSettingsHelper::restrictInputDataByName();
  settings["ErrorBars"] = IndirectSettingsHelper::externalPlotErrorBars();
  return settings;
}

void IndirectSettings::loadSettings() { m_presenter->loadSettings(); }

void IndirectSettings::closeSettings() {
  if (auto settingsWindow = window())
    settingsWindow->close();
}

} // namespace MantidQt::CustomInterfaces
