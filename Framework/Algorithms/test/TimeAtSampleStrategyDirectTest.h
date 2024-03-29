// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"

using namespace Mantid::Algorithms;

class TimeAtSampleStrategyDirectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyDirectTest *createSuite() { return new TimeAtSampleStrategyDirectTest(); }
  static void destroySuite(TimeAtSampleStrategyDirectTest *suite) { delete suite; }

  void test_L2_detector() {
    using namespace Mantid;
    using namespace Mantid::Geometry;

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    auto instrument = ws->getInstrument();

    auto sample = instrument->getSample();

    auto source = instrument->getSource();

    const size_t detectorIndex = 0; // detector workspace index.
    const double ei = 12;           // MeV

    const double L1 = source->getPos().distance(sample->getPos());

    TimeAtSampleStrategyDirect strategy(ws, ei);
    Correction correction = strategy.calculate(detectorIndex);

    const double shift = correction.factor;

    double expectedShift = L1 / std::sqrt(ei * 2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass);

    TSM_ASSERT_DELTA("L1 / (L1 + L2)", expectedShift, shift, 0.0000001);
  }
};
