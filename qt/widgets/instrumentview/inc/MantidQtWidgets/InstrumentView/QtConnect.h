// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0

#pragma once

#include <QObject>

namespace MantidQt::MantidWidgets {

struct QtConnect {
  virtual ~QtConnect() = default;

  virtual void connect(QObject *sender, const char *signal, QObject *receiver, const char *slot) const {
    QObject::connect(sender, signal, receiver, slot);
  }

  virtual void connect(QObject *sender, const char *signal, QObject *receiver, const char *slot,
                       Qt::ConnectionType type) const {
    QObject::connect(sender, signal, receiver, slot, type);
  }
};
} // namespace MantidQt::MantidWidgets
