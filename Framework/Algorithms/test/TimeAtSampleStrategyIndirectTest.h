#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_

#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
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
    TimeAtSampleStrategyIndirect strategy(ws);
    TS_ASSERT_THROWS(strategy.calculate(1 /*monitor index*/),
                     std::invalid_argument &);
  }
};

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_ */
