// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/QtConnect.h"

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {
class MockQtConnect : public QtConnect {
public:
  virtual ~MockQtConnect() = default;
  MOCK_METHOD(void, connect, (QObject *, const char *, QObject *, const char *), (const, override));
  MOCK_METHOD(void, connect, (QObject *, const char *, QObject *, const char *, Qt::ConnectionType), (const, override));
};
} // namespace MantidQt::MantidWidgets
