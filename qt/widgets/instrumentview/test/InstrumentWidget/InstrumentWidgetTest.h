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
#include "MockProjectionSurface.h"
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
  using GLMock = StrictMock<MockMantidGLWidget>;

  void setUp() override {
    FrameworkManager::Instance();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    AnalysisDataService::Instance().addOrReplace("test_ws", ws);
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_constructor() { auto instance = construct(makeSimple(), makeGL()); }

  void test_save_image_simple_widget() {
    const auto inputName = QString::fromStdString("testFilename");
    const auto expectedName = inputName + ".png";

    auto simpleMock = makeSimple();
    EXPECT_CALL(*simpleMock, saveToFile(expectedName)).Times(1);

    auto widget = construct(std::move(simpleMock), makeGL());
    widget.saveImage(inputName);
  }

  void test_update_instrument_detectors() {
    auto simpleMock = makeSimple();
    EXPECT_CALL(*simpleMock, updateDetectors()).Times(1);

    auto widget = construct(std::move(simpleMock), makeGL());
    widget.updateInstrumentDetectors();
  }

private:
  std::unique_ptr<SimpleMock> makeSimple() const { return std::make_unique<SimpleMock>(); }
  std::unique_ptr<GLMock> makeGL() const { return std::make_unique<GLMock>(); }

  InstrumentWidget construct(std::unique_ptr<SimpleMock> simpleMock, std::unique_ptr<GLMock> glMock) const {
    auto surfaceMock = std::make_shared<MockProjectionSurface>();
    EXPECT_CALL(*simpleMock, setSurface(_)).Times(1);
    EXPECT_CALL(*simpleMock, qtInstallEventFilter(_)).Times(1);
    EXPECT_CALL(*simpleMock, qtUpdate()).Times(1);

    EXPECT_CALL(*glMock, qtInstallEventFilter(_)).Times(1);
    EXPECT_CALL(*glMock, setBackgroundColor(_)).Times(1);
    EXPECT_CALL(*glMock, getSurface()).Times(24).WillRepeatedly(Return(surfaceMock));
    EXPECT_CALL(*glMock, setSurface(_)).Times(1);

    return InstrumentWidget("test_ws", nullptr, true, true, 0.0, 0.0, true, std::move(simpleMock), std::move(glMock));
  }
};
