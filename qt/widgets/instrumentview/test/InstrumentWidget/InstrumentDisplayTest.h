// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IGLDisplay.h"
#include "IQtDisplay.h"
#include "InstrumentDisplay.h"
#include "MockGLDisplay.h"
#include "MockProjectionSurface.h"
#include "MockQtDisplay.h"
#include "MockStackedLayout.h"

#include <QObject>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace testing;

class InstrumentDisplayTest : public CxxTest::TestSuite {
public:
  static InstrumentDisplayTest *createSuite() { return new InstrumentDisplayTest(); }
  static void destroySuite(InstrumentDisplayTest *suite) { delete suite; }

  using QtMock = MockQtDisplay;
  using GLMock = MockGLDisplay;

  void test_install_event_filter() {
    auto qtMock = makeQtDisplay();
    auto glMock = makeGLDisplay();
    EXPECT_CALL(*qtMock, qtInstallEventFilter(IsNull())).Times(1);
    EXPECT_CALL(*glMock, qtInstallEventFilter(IsNull())).Times(1);
    auto instDisplay = makeInstDisplay(std::move(glMock), std::move(qtMock));
    instDisplay.installEventFilter(nullptr);
  }

  void test_add_widget_in_constructor() {
    auto qtMock = makeQtDisplay();
    auto glMock = makeGLDisplay();

    auto layoutMock = std::make_unique<MockStackedLayout>();
    EXPECT_CALL(*layoutMock, addWidget(Eq(glMock.get()))).Times(1);
    EXPECT_CALL(*layoutMock, addWidget(Eq(qtMock.get()))).Times(1);

    InstrumentDisplay fixture(nullptr, std::move(glMock), std::move(qtMock), std::move(layoutMock));
  }

  void test_set_surface() {
    auto glMock = makeGLDisplay();
    auto qtMock = makeQtDisplay();

    EXPECT_CALL(*glMock, setSurface(_)).Times(1);
    EXPECT_CALL(*glMock, qtUpdate()).Times(1);
    EXPECT_CALL(*qtMock, setSurface(_)).Times(1);
    EXPECT_CALL(*qtMock, qtUpdate()).Times(1);

    auto projection = std::make_shared<MockProjectionSurface>();

    auto inst = makeInstDisplay(std::move(glMock), std::move(qtMock));
    inst.setSurface(projection);
  }

private:
  std::unique_ptr<QtMock> makeQtDisplay() const { return std::make_unique<QtMock>(); }
  std::unique_ptr<GLMock> makeGLDisplay() { return std::make_unique<GLMock>(); }
  InstrumentDisplay makeInstDisplay(std::unique_ptr<GLMock> glMock, std::unique_ptr<QtMock> qtMock) {
    return InstrumentDisplay(nullptr, std::move(glMock), std::move(qtMock), nullptr);
  }
};