// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettings.h"
#include "IndirectInterface.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtIcons/Icon.h"

constexpr auto SETTINGS_ICON = "mdi.settings";
#endif

namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectSettings)

IndirectSettings::IndirectSettings(QWidget *parent)
    : MantidQt::API::UserSubWindow(parent) {
  m_uiForm.setupUi(this);
}

QIcon IndirectSettings::icon() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  return QIcon(":/configure.png");
#else
  return Icons::getIcon(SETTINGS_ICON);
#endif
}

void IndirectSettings::initLayout() {
  m_presenter = std::make_unique<IndirectSettingsPresenter>(this);

  auto centralWidget = m_uiForm.centralWidget->layout();
  centralWidget->addWidget(m_presenter->getView());

  connect(m_presenter.get(), SIGNAL(applySettings()), this,
          SIGNAL(applySettings()));
  connect(m_presenter.get(), SIGNAL(closeSettings()), this,
          SLOT(closeSettings()));
}

void IndirectSettings::otherUserSubWindowCreated(
    QPointer<UserSubWindow> window) {
  connectIndirectInterface(window);
}

void IndirectSettings::otherUserSubWindowCreated(
    QList<QPointer<UserSubWindow>> &windows) {
  for (auto &window : windows)
    connectIndirectInterface(window);
}

void IndirectSettings::connectIndirectInterface(
    QPointer<UserSubWindow> window) {
  if (auto indirectInterface = dynamic_cast<IndirectInterface *>(window.data()))
    connect(m_presenter.get(), SIGNAL(applySettings()), indirectInterface,
            SLOT(applySettings()));
}

std::map<std::string, QVariant> IndirectSettings::getSettings() const {
  std::map<std::string, QVariant> settings;
  settings["RestrictInput"] = m_presenter->getSetting("restrict-input-by-name");
  settings["ErrorBars"] = m_presenter->getSetting("plot-error-bars");
  return settings;
}

void IndirectSettings::loadSettings() { m_presenter->loadSettings(); }

void IndirectSettings::closeSettings() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  getDockedOrFloatingWindow()->close();
#else
  if (auto settingsWindow = window())
    settingsWindow->close();
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
QWidget *IndirectSettings::getDockedOrFloatingWindow() const {
  auto widget = parentWidget();
  while (widget) {
    auto const className = std::string(widget->metaObject()->className());
    if (className == "DockedWindow" || className == "FloatingWindow")
      return widget;
    widget = widget->parentWidget();
  }
  return window();
}
#endif

} // namespace CustomInterfaces
} // namespace MantidQt
