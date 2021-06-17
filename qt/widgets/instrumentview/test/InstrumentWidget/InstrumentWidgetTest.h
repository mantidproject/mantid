// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IMantidGLWidget.h"
#include "ISimpleWidget.h"

#include "MockMantidGLWidget.h"
#include "MockSimpleWidget.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::MantidWidgets;
using testing::StrictMock;
class InstrumentWidgetTest : public CxxTest::TestSuite {
public:
  using MockedSimple = StrictMock<MockSimpleWidget>;

  void test_constructor() {
    auto simpleFixture = std::make_unique<MockedSimple>();

    // InstrumentWidget(args,  std::move(simpleFixture));
    //    EXPECT_CALL(*simpleFixture, getSurface()).Times(1);
  }
};
