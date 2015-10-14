#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTICTEST_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTICTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include "MantidAlgorithms/TimeAtSampleStrategyElastic.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::Algorithms;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class TimeAtSampleStrategyElasticTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyElasticTest *createSuite() {
    return new TimeAtSampleStrategyElasticTest();
  }
  static void destroySuite(TimeAtSampleStrategyElasticTest *suite) {
    delete suite;
  }

  void test_L2_detector() {

    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    auto instrument = ws->getInstrument();

    auto sample = instrument->getSample();

    auto source = instrument->getSource();

    const size_t detectorIndex = 0; // detector workspace index.
    auto detector = ws->getDetector(detectorIndex);

    const double L1 = source->getPos().distance(sample->getPos());

    TimeAtSampleStrategyElastic strategy(ws);
    Correction correction = strategy.calculate(detectorIndex);

    const double ratio = correction.factor;

    TSM_ASSERT_EQUALS("L1 / (L1 + L2)",
                      L1 / (L1 + sample->getPos().distance(detector->getPos())),
                      ratio);
  }

  void test_L2_monitor() {

    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    auto instrument = ws->getInstrument();

    auto sample = instrument->getSample();

    auto source = instrument->getSource();

    const V3D &beamDir =
        instrument->getReferenceFrame()->vecPointingAlongBeam();

    const size_t monitorIndex = 1; // monitor workspace index.
    auto monitor = ws->getDetector(monitorIndex);

    const double L1 = source->getPos().distance(sample->getPos());

    TimeAtSampleStrategyElastic strategy(ws);
    Correction correction = strategy.calculate(monitorIndex);

    const double ratio = correction.factor;

    TSM_ASSERT_EQUALS(
        "L1/L1m",
        std::abs(L1 /
                 beamDir.scalar_prod(source->getPos() - monitor->getPos())),
        ratio);
  }
};

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTICTEST_H_ */
