// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../Common/Detector.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace ::testing;
using namespace MantidQt::CustomInterfaces::ISISReflectometry;

namespace {

Mantid::API::MatrixWorkspace_sptr createLinearDetectorWorkspace() {
  return WorkspaceCreationHelper::create2DWorkspace(1, 1);
}

Mantid::API::MatrixWorkspace_sptr createRectangularDetectorWorkspace() {
  auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
  auto rectangularInstrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
  ws->setInstrument(rectangularInstrument);
  return ws;
}

} // namespace

class DetectorTest : public CxxTest::TestSuite {
public:
  static DetectorTest *createSuite() { return new DetectorTest(); }
  static void destroySuite(DetectorTest *suite) { delete suite; }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void test_has_linear_detector_returns_true_when_the_workspace_has_linear_detector() {
    auto ws = createLinearDetectorWorkspace();
    TS_ASSERT(hasLinearDetector(ws));
  }

  void test_has_linear_detector_returns_false_when_the_workspace_has_rectangular_detector() {
    auto ws = createRectangularDetectorWorkspace();
    TS_ASSERT(!hasLinearDetector(ws));
  }
};
