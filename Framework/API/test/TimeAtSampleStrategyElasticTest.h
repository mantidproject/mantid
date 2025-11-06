// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TimeAtSampleStrategy.h"
#include "MantidAPI/TimeAtSampleStrategyElastic.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class TimeAtSampleStrategyElasticTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyElasticTest *createSuite() { return new TimeAtSampleStrategyElasticTest(); }
  static void destroySuite(TimeAtSampleStrategyElasticTest *suite) { delete suite; }

  void test_L2_detector() {
    constexpr size_t detectorIndex = 0; // detector workspace index.

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    auto instrument = ws->getInstrument();

    auto sample = instrument->getSample();

    auto source = instrument->getSource();

    const auto &spectrumInfo = ws->spectrumInfo();

    const double L1 = spectrumInfo.l1();

    TimeAtSampleStrategyElastic strategy(ws);
    Correction correction = strategy.calculate(detectorIndex);

    TSM_ASSERT_EQUALS("L1 / (L1 + L2)", L1 / (L1 + spectrumInfo.l2(detectorIndex)), correction.factor);
    TS_ASSERT_EQUALS(0., correction.offset);
  }

  void test_L2_monitor() {
    const size_t monitorIndex = 1; // monitor workspace index.

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    auto instrument = ws->getInstrument();

    auto sample = instrument->getSample();

    auto source = instrument->getSource();

    const V3D &beamDir = instrument->getReferenceFrame()->vecPointingAlongBeam();

    auto monitor = ws->getDetector(monitorIndex);

    const double L1 = source->getPos().distance(sample->getPos());

    TimeAtSampleStrategyElastic strategy(ws);
    Correction correction = strategy.calculate(monitorIndex);

    TSM_ASSERT_EQUALS("L1/L1m", std::abs(L1 / beamDir.scalar_prod(source->getPos() - monitor->getPos())),
                      correction.factor);
    TS_ASSERT_EQUALS(0., correction.offset);
  }
};
