// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IMantidGLWidget.h"
#include "ISimpleWidget.h"
#include "InstrumentWidget.h"

#include "MockMantidGLWidget.h"
#include "MockSimpleWidget.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace testing;
using Mantid::API::AnalysisDataService;
using Mantid::API::FrameworkManager;

class InstrumentWidgetTest : public CxxTest::TestSuite {
public:
  static InstrumentWidgetTest *createSuite() { return new InstrumentWidgetTest(); }
  static void destroySuite(InstrumentWidgetTest *suite) { delete suite; }

  using SimpleMock = StrictMock<MockSimpleWidget>;

  void setUp() override {
    FrameworkManager::Instance();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    AnalysisDataService::Instance().addOrReplace("test_ws", ws);
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_constructor_simple_widget() { auto instance = construct(makeSimple()); }

  void test_save_image() {
    const auto inputName = QString::fromStdString("testFilename");
    const auto expectedName = inputName + ".png";

    auto simpleMock = makeSimple();
    EXPECT_CALL(*simpleMock, saveToFile(expectedName)).Times(1);

    auto widget = construct(std::move(simpleMock));
    widget.saveImage(inputName);
  }

private:
  std::unique_ptr<SimpleMock> makeSimple() const { return std::make_unique<SimpleMock>(); }

  InstrumentWidget construct(std::unique_ptr<SimpleMock> mock) const {
    EXPECT_CALL(*mock, setSurface(_)).Times(1);
    EXPECT_CALL(*mock, updateView(IsTrue())).Times(1);
    EXPECT_CALL(*mock, qtInstallEventFilter(_)).Times(1);
    EXPECT_CALL(*mock, qtUpdate()).Times(1);

    return InstrumentWidget("test_ws", nullptr, true, true, 0.0, 0.0, true, std::move(mock));
  }
};
