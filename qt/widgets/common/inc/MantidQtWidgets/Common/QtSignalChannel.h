// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <Poco/Channel.h>
#include <QObject>

namespace MantidQt {
namespace MantidWidgets {
//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
class Message;

/**
Provides a translation layer that takes a Poco::Message and converts it
to a Qt signal.
*/
class EXPORT_OPT_MANTIDQT_COMMON QtSignalChannel : public QObject, public Poco::Channel {
  Q_OBJECT

public:
  /// Default constructor
  QtSignalChannel(const QString &source = "");
  /// Destructor
  ~QtSignalChannel() override;

  /// If set, only Mantid log messages from this source are emitted
  void setSource(const QString &source);
  /// Get the current source are emitted
  inline const QString &source() const { return m_source; }

  /// Converts the Poco::Message to a Qt signal
  void log(const Poco::Message &msg) override;

public slots:
  /// Set the log level for all loggers
  void setGlobalLogLevel(int level);

signals:
  // Emitted when a Poco log message is received in this channel
  void messageReceived(const Message &msg);

private:
  Q_DISABLE_COPY(QtSignalChannel)

  /// Optional source (use std::string to avoid conversion in comparison)
  QString m_source;
};
} // namespace MantidWidgets
} // namespace MantidQt
