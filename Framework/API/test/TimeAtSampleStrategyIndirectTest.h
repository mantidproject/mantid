// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TimeAtSampleStrategyElastic.h"
#include "MantidAPI/TimeAtSampleStrategyIndirect.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::TimeAtSampleStrategyElastic;
using Mantid::API::TimeAtSampleStrategyIndirect;

class TimeAtSampleStrategyIndirectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyIndirectTest *createSuite() { return new TimeAtSampleStrategyIndirectTest(); }
  static void destroySuite(TimeAtSampleStrategyIndirectTest *suite) { delete suite; }

  void test_L2_detector_no_efixed() {
    const size_t detectorIndex = 0; // detector workspace index.

    // there is no efixed so this will generate an exception
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    TimeAtSampleStrategyIndirect strategy(ws);
    TSM_ASSERT_THROWS("No Efixed should throw", strategy.calculate(detectorIndex), const std::runtime_error &);
  }

  void test_L2_detector() {
    constexpr size_t detectorIndex = 0;
    constexpr double efixed = 5.0; // meV
    constexpr double TWO_MEV_OVER_MASS = 2. * Mantid::PhysicalConstants::meV / Mantid::PhysicalConstants::NeutronMass;
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
    // set efixed
    auto instrument = ws->getInstrument();
    auto detector = ws->getDetector(detectorIndex);
    auto &paramMap = ws->instrumentParameters();
    paramMap.addDouble(detector.get(), "Efixed", efixed); // meV

    const auto &spectrumInfo = ws->spectrumInfo();

    const double l2 = spectrumInfo.l2(detectorIndex);
    const double expectedShift = -1. * l2 / sqrt(efixed * TWO_MEV_OVER_MASS);

    TimeAtSampleStrategyIndirect strategy(ws);
    const auto correction = strategy.calculate(detectorIndex);

    TS_ASSERT_EQUALS(1., correction.factor);
    TSM_ASSERT_EQUALS("- L2*sqrt(m/2/Ef)", expectedShift, correction.offset);
  }

  void test_L2_monitors() {
    constexpr size_t monitorIndex = 1; // monitor workspace index.

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(); // workspace has
                                                                                       // monitors

    // indirect monitor is the same as elastic monitor
    const auto correctionElastic = TimeAtSampleStrategyElastic(ws).calculate(monitorIndex);
    const auto correctionIndirect = TimeAtSampleStrategyIndirect(ws).calculate(monitorIndex);

    TS_ASSERT_DELTA(correctionElastic.factor, correctionIndirect.factor, 0.0000001);
    TS_ASSERT_DELTA(correctionElastic.offset, correctionIndirect.offset, 0.0000001);
  }
};
