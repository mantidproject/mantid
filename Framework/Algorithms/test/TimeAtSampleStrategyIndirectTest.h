// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_

#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::Algorithms::TimeAtSampleStrategyIndirect;

class TimeAtSampleStrategyIndirectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyIndirectTest *createSuite() {
    return new TimeAtSampleStrategyIndirectTest();
  }
  static void destroySuite(TimeAtSampleStrategyIndirectTest *suite) {
    delete suite;
  }

  void test_break_on_monitors() {
    auto ws = WorkspaceCreationHelper::
        create2DWorkspaceWithReflectometryInstrument(); // workspace has
                                                        // monitors

    const size_t monitorIndex = 1; // monitor workspace index.
    const auto &instrument = ws->getInstrument();
    auto sample = instrument->getSample();
    auto source = instrument->getSource();

    const auto &beamDir =
        instrument->getReferenceFrame()->vecPointingAlongBeam();

    auto monitor = ws->getDetector(monitorIndex);

    const double L1 = source->getPos().distance(sample->getPos());

    TimeAtSampleStrategyIndirect strategy(ws);
    const auto correction = strategy.calculate(monitorIndex);

    TSM_ASSERT_EQUALS("L1/L1m",
                      std::abs(L1 / beamDir.scalar_prod(source->getPos() -
                                                        monitor->getPos())),
                      correction.factor);
    TS_ASSERT_EQUALS(0., correction.offset);
  }
};

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_ */
