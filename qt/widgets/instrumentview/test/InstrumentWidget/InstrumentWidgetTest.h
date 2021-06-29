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
#include <memory>

using namespace MantidQt::MantidWidgets;

class InstrumentWidgetTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    std::unique_ptr<ISimpleWidget> simpleFixture = std::make_unique<MockSimpleWidget>();
    std::unique_ptr<IMantidGLWidget> glFixture = std::make_unique<MockMantidGLWidget>();
  }

  void test_stub() { TS_ASSERT(true); }
};
