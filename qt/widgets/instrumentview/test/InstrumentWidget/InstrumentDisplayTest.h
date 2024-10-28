// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/IGLDisplay.h"
#include "MantidQtWidgets/InstrumentView/IQtDisplay.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"

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
    auto layoutMock = makeLayout();

    EXPECT_CALL(*layoutMock, addWidget(Eq(glMock.get()))).Times(1);
    EXPECT_CALL(*layoutMock, addWidget(Eq(qtMock.get()))).Times(1);

    InstrumentDisplay fixture(nullptr, std::move(glMock), std::move(qtMock), std::move(layoutMock));
  }

  void test_get_surface() {
    auto glMock = makeGLDisplay();
    auto qtMock = makeQtDisplay();

    EXPECT_CALL(*glMock, getSurface()).Times(1);

    auto inst = makeInstDisplay(std::move(glMock), std::move(qtMock));
    inst.getSurface();
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

  void test_get_set_surface_returns_same() {
    auto glMock = makeGLDisplay();
    auto qtMock = makeQtDisplay();

    EXPECT_CALL(*glMock, setSurface(_)).Times(1);
    EXPECT_CALL(*glMock, qtUpdate()).Times(1);
    EXPECT_CALL(*qtMock, setSurface(_)).Times(1);
    EXPECT_CALL(*qtMock, qtUpdate()).Times(1);

    auto projection = std::make_shared<MockProjectionSurface>();
    EXPECT_CALL(*glMock, getSurface()).Times(1).WillOnce(Return(projection));
    auto inst = makeInstDisplay(std::move(glMock), std::move(qtMock));
    inst.setSurface(projection);
    TS_ASSERT_EQUALS(projection, inst.getSurface());
  }

  void test_update_view_gl() {
    for (bool picking : {true, false}) {
      auto glMock = makeGLDisplay();
      auto qtMock = makeQtDisplay();
      auto layoutMock = makeLayout();

      expectCurrentWidget(layoutMock.get(), glMock.get());
      EXPECT_CALL(*glMock, updateView(picking)).Times(1);

      auto inst = makeInstDisplay(std::move(glMock), std::move(qtMock), std::move(layoutMock));
      inst.updateView(picking);
    }
  }

  void test_update_view_qt() {
    for (bool picking : {true, false}) {
      auto glMock = makeGLDisplay();
      auto qtMock = makeQtDisplay();
      auto layoutMock = makeLayout();

      expectCurrentWidget(layoutMock.get(), qtMock.get());
      EXPECT_CALL(*qtMock, updateView(picking)).Times(1);

      auto inst = makeInstDisplay(std::move(glMock), std::move(qtMock), std::move(layoutMock));
      inst.updateView(picking);
    }
  }

private:
  std::unique_ptr<QtMock> makeQtDisplay() const { return std::make_unique<QtMock>(); }
  std::unique_ptr<GLMock> makeGLDisplay() { return std::make_unique<GLMock>(); }
  std::unique_ptr<MockStackedLayout> makeLayout() { return std::make_unique<MockStackedLayout>(); }
  InstrumentDisplay makeInstDisplay(std::unique_ptr<GLMock> glMock, std::unique_ptr<QtMock> qtMock,
                                    std::unique_ptr<MockStackedLayout> layoutMock = nullptr) {
    return InstrumentDisplay(nullptr, std::move(glMock), std::move(qtMock), std::move(layoutMock));
  }

  template <typename T> void expectCurrentWidget(MockStackedLayout *layoutMock, T *displayMock) {
    EXPECT_CALL(*layoutMock, currentWidget()).Times(1).WillOnce(Return(displayMock));
  }
};
