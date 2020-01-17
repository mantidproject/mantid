// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/NotificationService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include <QApplication>

namespace MantidQt {
namespace MantidWidgets {

// Key for the "normalize data to bin width" plot option
const std::string NotificationService::NOTIFICATIONS_ENABLED_KEY =
    "Notifications.Enabled";

void NotificationService::showMessage(const QString &title,
                                      const QString &message, MessageIcon icon,
                                      int millisecondsTimeoutHint) {
  if (isEnabled() && isSupportedByOS()) {
    QSystemTrayIcon sysTrayIcon(qApp);
    // get the window icon for the app
    QIcon windowIcon = qApp->windowIcon();
    // if no icon is set then use the mantid icon
    if (windowIcon.isNull()) {
      try {
        windowIcon = QIcon(":/images/MantidIcon.ico");
      } catch (std::exception) {
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

bool NotificationService::isEnabled() {
  bool retVal = false;
  try {
    retVal = Mantid::Kernel::ConfigService::Instance()
                 .getValue<bool>(NOTIFICATIONS_ENABLED_KEY)
                 .get_value_or(true);
  } catch (Mantid::Kernel::Exception::FileError) {
    // The Config Service could not find the properties file
    // Disable notifications
    retVal = false;
  } catch (Poco::ExistsException) {
    // The Config Service could not find the properties file
    // Disable notifications
    retVal = false;
  }
  return retVal;
}

bool NotificationService::isSupportedByOS() {
  return QSystemTrayIcon::supportsMessages();
}

} // namespace MantidWidgets
} // namespace MantidQt
