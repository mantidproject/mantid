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
#include "MockQtDisplay.h"
#include "MockStackedLayout.h"

#include <QObject>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace testing;

class FakeInstrumentDisplay : public InstrumentDisplay {
public:
  FakeInstrumentDisplay(MockStackedLayout &mockLayout, std::unique_ptr<IGLDisplay> glDisplay,
                        std::unique_ptr<IQtDisplay> qtDisplay)
      : InstrumentDisplay(std::move(glDisplay), std::move(qtDisplay), nullptr), m_mockLayout(mockLayout) {}
  IStackedLayout *createLayout(QWidget *) const override { return &m_mockLayout; }

private:
  MockStackedLayout &m_mockLayout;
};

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

    StrictMock<MockStackedLayout> mock;
    EXPECT_CALL(mock, addWidget(glMock.get())).Times(1);
    EXPECT_CALL(mock, addWidget(qtMock.get())).Times(1);

    FakeInstrumentDisplay fixture(mock, std::move(glMock), std::move(qtMock));
  }

private:
  std::unique_ptr<QtMock> makeQtDisplay() const { return std::make_unique<QtMock>(); }
  std::unique_ptr<GLMock> makeGLDisplay() { return std::make_unique<GLMock>(); }
  InstrumentDisplay makeInstDisplay(std::unique_ptr<GLMock> glMock, std::unique_ptr<QtMock> qtMock) {
    return InstrumentDisplay(std::move(glMock), std::move(qtMock), nullptr);
  }
};