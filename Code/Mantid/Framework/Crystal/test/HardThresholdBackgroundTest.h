#ifndef MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUNDTEST_H_
#define MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects::MDEventsTestHelper;

class HardThresholdBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HardThresholdBackgroundTest *createSuite() { return new HardThresholdBackgroundTest(); }
  static void destroySuite( HardThresholdBackgroundTest *suite ) { delete suite; }


  void test_isBackground()
  {
    const double threshold = 1;
    MDHistoWorkspace_sptr ws = makeFakeMDHistoWorkspace(threshold, 1, 1);
    auto iterator = ws->createIterator(NULL);

    HardThresholdBackground strategy(threshold, Mantid::API::NoNormalization);

    TS_ASSERT(strategy.isBackground(iterator));
  }

};


#endif /* MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUNDTEST_H_ */
