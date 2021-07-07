// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IGLDisplay.h"
#include "IInstrumentDisplay.h"
#include "IQtDisplay.h"

#include <gmock/gmock.h>
#include <memory>

class IStackedLayout;
class QStackedLayout;
class QWidget;

namespace MantidQt::MantidWidgets {

class MockInstrumentDisplay : public IInstrumentDisplay {
public:
  virtual ~MockInstrumentDisplay() = default;
  MOCK_METHOD(int, currentIndex, (), (const, override));
  MOCK_METHOD(QWidget *, currentWidget, (), (const, override));
  MOCK_METHOD(void, setCurrentIndex, (int), (const, override));
  MOCK_METHOD(IGLDisplay *, getGLDisplay, (), (const, override));
  MOCK_METHOD(IQtDisplay *, getQtDisplay, (), (const, override));
  MOCK_METHOD(void, installEventFilter, (QObject * obj), (override));
  MOCK_METHOD(void, setSurface, (ProjectionSurface *), (override));
};
} // namespace MantidQt::MantidWidgets