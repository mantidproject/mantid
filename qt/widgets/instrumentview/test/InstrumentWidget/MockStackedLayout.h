// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/IStackedLayout.h"

#include <gmock/gmock.h>

class QWidget;

namespace MantidQt::MantidWidgets {
class MockStackedLayout : public IStackedLayout {
public:
  virtual ~MockStackedLayout() = default;
  MOCK_METHOD(int, addWidget, (QWidget *), (override));
  MOCK_METHOD(int, currentIndex, (), (const, override));
  MOCK_METHOD(QWidget *, currentWidget, (), (const, override));
  MOCK_METHOD(void, setCurrentIndex, (int), (override));
};
} // namespace MantidQt::MantidWidgets
