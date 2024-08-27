// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/NotificationService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Timer.h"
#include <QApplication>

namespace MantidQt::MantidWidgets {

// Key for the "Notifications.Enabled" option
const std::string NotificationService::NOTIFICATIONS_ENABLED_KEY = "Notifications.Enabled";
// Minimum number os seconds between identical warnings
const float NotificationService::MIN_SECONDS_BETWEEN_IDENTICAL_NOTIFICATIONS = 5.0;

// setup static variables
QString NotificationService::g_lastMessage = "";
QString NotificationService::g_lastTitle = "";
Mantid::Kernel::Timer NotificationService::g_timer;

void NotificationService::showMessage(const QString &title, const QString &message, MessageIcon icon,
                                      int millisecondsTimeoutHint) {
  if (isEnabled() && isSupportedByOS()) {
    if ((g_lastMessage != message) || (g_lastTitle != title) ||
        (g_timer.elapsed_no_reset() > MIN_SECONDS_BETWEEN_IDENTICAL_NOTIFICATIONS)) {
      // remeber the last message details
      g_lastMessage = message;
      g_lastTitle = title;
      g_timer.reset();

      QSystemTrayIcon sysTrayIcon(qApp);
      // get the window icon for the app
      QIcon windowIcon = qApp->windowIcon();
      // if no icon is set then use the mantid icon
      if (windowIcon.isNull()) {
        try {
          windowIcon = QIcon(":/images/MantidIcon.ico");
        } catch (const std::exception &) {
          // if we cannot use the embedded icon, use a blank one
          windowIcon = QIcon(QPixmap(32, 32));
        }
      }
      // set this as the window icon otherwise you get a warning on the console
      sysTrayIcon.setIcon(windowIcon);

      sysTrayIcon.show();
      sysTrayIcon.showMessage(title, message, icon, millisecondsTimeoutHint);

      sysTrayIcon.hide();
    }
  }
}

bool NotificationService::isEnabled() {
  bool retVal = false;
  try {
    retVal = Mantid::Kernel::ConfigService::Instance().getValue<bool>(NOTIFICATIONS_ENABLED_KEY).value_or(true);
  } catch (const Mantid::Kernel::Exception::FileError &) {
    // The Config Service could not find the properties file
    // Disable notifications
    retVal = false;
  } catch (const Poco::ExistsException &) {
    // The Config Service could not find the properties file
    // Disable notifications
    retVal = false;
  }
  return retVal;
}

bool NotificationService::isSupportedByOS() { return QSystemTrayIcon::supportsMessages(); }

} // namespace MantidQt::MantidWidgets
