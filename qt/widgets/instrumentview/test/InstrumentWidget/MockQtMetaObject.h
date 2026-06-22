// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/QtMetaObject.h"

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {
class MockQtMetaObject : public QtMetaObject {
public:
  virtual ~MockQtMetaObject() = default;

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, Qt::ConnectionType, QtInvokeReturnArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument, QtInvokeArgument),
              (const, override));

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, QtInvokeReturnArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument),
              (const, override));

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, Qt::ConnectionType, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument),
              (const, override));

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument, QtInvokeArgument,
               QtInvokeArgument),
              (const, override));
};
} // namespace MantidQt::MantidWidgets
