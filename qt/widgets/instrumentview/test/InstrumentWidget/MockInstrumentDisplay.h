// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/IGLDisplay.h"
#include "MantidQtWidgets/InstrumentView/IInstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/IQtDisplay.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

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
  MOCK_METHOD(ProjectionSurface_sptr, getSurface, (), (const, override));
  MOCK_METHOD(void, setSurfaceProxy, (ProjectionSurface_sptr));
  MOCK_METHOD(void, updateView, (bool), (override));

  void setSurface(ProjectionSurface_sptr surface) override {
    setSurfaceProxy(surface);
    m_surface = surface;
  }

private:
  // We need to hold onto the sptr, as the calling class expects
  // this class to manage lifetimes
  std::shared_ptr<ProjectionSurface> m_surface;
};
} // namespace MantidQt::MantidWidgets
