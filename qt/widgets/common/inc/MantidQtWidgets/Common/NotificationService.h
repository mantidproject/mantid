// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_NOTIFICATIONSERVICE_H_
#define MANTIDQTAPI_NOTIFICATIONSERVICE_H_

#include "DllOption.h"
#include <QObject>
#include <QSystemTrayIcon>

/** The ase class from which mantid custom widgets are derived it contains
 *  some useful functions
 */
namespace MantidQt {
namespace MantidWidgets {
/**
This is a singleton providing a notification service for the Mantid Qt based
applications. This is just a thin simplistic wrapper around QSystemTray
*/
class EXPORT_OPT_MANTIDQT_COMMON NotificationService : public QObject {
  Q_OBJECT

public:
  // Our Notification icons are the same as Qt's.
  using MessageIcon = QSystemTrayIcon::MessageIcon;

  /// Default constructor
  NotificationService() : QObject() {}

  /// Display a notification
  static void showMessage(const QString &title, const QString &message,
                          MessageIcon icon = MessageIcon::Information,
                          int millisecondsTimeoutHint = 10000);

  /// Is the notification service enabled through the config service?
  static bool isEnabled();

  static const std::string NOTIFICATIONSENABLEDKEY;

  /// Are notifications supported by this OS?
  static bool isSupportedByOS();
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTAPI_NOTIFICATIONSERVICE_H_
