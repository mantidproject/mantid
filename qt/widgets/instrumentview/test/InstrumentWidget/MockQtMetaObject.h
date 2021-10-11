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
              (QObject *, const char *, Qt::ConnectionType, QGenericReturnArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument, QGenericArgument),
              (const, override));

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, QGenericReturnArgument, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument),
              (const, override));

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, Qt::ConnectionType, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument),
              (const, override));

  MOCK_METHOD(bool, invokeMethod,
              (QObject *, const char *, QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument, QGenericArgument,
               QGenericArgument),
              (const, override));
};
} // namespace MantidQt::MantidWidgets
