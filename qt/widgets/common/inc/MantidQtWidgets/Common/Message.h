// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MESSAGE_H_
#define MESSAGE_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidKernel/Logger.h" // So we can match the Logger priority

#include <QMetaType>
#include <QObject>
#include <QString>

//----------------------------------------------------------
// Forward declarations
//----------------------------------------------------------

namespace MantidQt {
namespace MantidWidgets {
/** @class Message
 * Provides a simple binding of a text message with a priority
 */
class EXPORT_OPT_MANTIDQT_COMMON Message : public QObject {
  // No Q_Object macro by design

public:
  /// Priority matches Mantid Logger priority
  using Priority = Mantid::Kernel::Logger::Priority;

  /// Default constuctor required by Qt meta-type system
  Message();
  /// Construct a message from a QString with a given priority (default=notice)
  Message(const QString &text, Priority priority = Priority::PRIO_NOTICE,
          QString scriptPath = "");
  /// Construct a message from a std::string with a given priority
  /// (default=notice)
  Message(const std::string &text, Priority priority = Priority::PRIO_NOTICE,
          QString scriptPath = "");
  /// Construct a message from a c-style string and a given priority
  /// (default=notice)
  Message(const char *text, Priority priority = Priority::PRIO_NOTICE,
          QString scriptPath = "");
  /// Copy constructor
  Message(const Message &msg);

public:
  /// @returns The message text
  inline QString text() const { return m_text; }
  /// @returns The message priority
  inline Priority priority() const { return m_priority; }
  /// @returns The name of the script the message came from
  inline QString scriptPath() const { return m_scriptPath; }
  /// Set the script name. This is useful if a script is renamed
  void setScriptPath(const QString &scriptPath) { m_scriptPath = scriptPath; }

private:
  QString m_text;
  Priority m_priority;
  QString m_scriptPath;
};
} // namespace MantidWidgets
} // namespace MantidQt

/// Required to operate in signals/slots
Q_DECLARE_METATYPE(MantidQt::MantidWidgets::Message)

#endif // MESSAGE_H_
