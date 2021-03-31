// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QObject>
#include <QSystemTrayIcon>

// namespace Mantid
namespace Mantid {
namespace Kernel {
// forward declaration
class Timer;
} // namespace Kernel
} // namespace Mantid

/** The base class from which mantid custom widgets are derived contains
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
  static void showMessage(const QString &title, const QString &message, MessageIcon icon = MessageIcon::Information,
                          int millisecondsTimeoutHint = 5000);

  /// Is the notification service enabled through the config service?
  static bool isEnabled();

  static const std::string NOTIFICATIONS_ENABLED_KEY;
  static const float MIN_SECONDS_BETWEEN_IDENTICAL_NOTIFICATIONS;

  /// Are notifications supported by this OS?
  static bool isSupportedByOS();

private:
  static QString g_lastMessage;
  static QString g_lastTitle;
  static Mantid::Kernel::Timer g_timer;
};
} // namespace MantidWidgets
} // namespace MantidQt
