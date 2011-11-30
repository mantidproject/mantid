#ifndef MANTID_ALGORITHMS_WEIGHTINGSTRATEGYTEST_H_
#define MANTID_ALGORITHMS_WEIGHTINGSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/WeightingStrategy.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class WeightingStrategyTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WeightingStrategyTest *createSuite() { return new WeightingStrategyTest(); }
  static void destroySuite( WeightingStrategyTest *suite ) { delete suite; }


  void testNullWeightingStrategyAtRadiusThrows()
  {
    NullWeighting strategy;
    double distance = 0;
    TSM_ASSERT_THROWS("NullWeighting should always throw in usage", strategy.weightAt(distance), std::runtime_error);
  }

  void testNullWeightingStrategyRectangularThrows()
  {
    NullWeighting strategy;
    int adjX = 0;
    int adjY = 0;
    int ix = 0;
    int iy = 0;
    TSM_ASSERT_THROWS("NullWeighting should always throw in usage", strategy.weightAt(adjX, adjY, ix, iy), std::runtime_error);
  }

  void testFlatWeightingStrategyAtRadius()
  {
    FlatWeighting strategy;
    double distanceA = 0;
    double distanceB = 1000;
    TSM_ASSERT_EQUALS("FlatWeighting Should be distance insensitive", 1, strategy.weightAt(distanceA));
    TSM_ASSERT_EQUALS("FlatWeighting Should be distance insensitive", 1, strategy.weightAt(distanceB));
  }

  void testFlatWeightingStrategyRectangular()
  {
    FlatWeighting strategy;
    int adjX = 0;
    int adjY = 0;
    int ix = 0;
    int iy = 0;
    TSM_ASSERT_EQUALS("FlatWeighting Should be 1", 1, strategy.weightAt(adjX, ix, adjY, iy));
  }

  void testLinearWeightingAtRadius()
  {
    double cutOff = 2;
    LinearWeighting strategy(cutOff);

    double distance = 0;
    TSM_ASSERT_EQUALS("LinearWeighting should give full weighting at origin", 1, strategy.weightAt(distance));
    distance = 1;
    TSM_ASSERT_EQUALS("LinearWeighting should give 0.5 weighting at 1/2 radius", 0.5, strategy.weightAt(distance));
    distance = cutOff; //2
    TSM_ASSERT_EQUALS("LinearWeighting should give zero weighting at cutoff", 0, strategy.weightAt(distance));
  }

  void testLinearWeightingRectangular()
  {
    double cutOff = 0; //Doesn't matter what the cut off is.
    LinearWeighting strategy(cutOff);

    int adjX = 2;
    int adjY = 2; 

    int ix = 2; int iy = 2;
    TSM_ASSERT_EQUALS("Top-Right not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = 2;
    TSM_ASSERT_EQUALS("Top-Left not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Right not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Left not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 0; iy = 0;
    TSM_ASSERT_EQUALS("Center not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 1; iy = 1;
    TSM_ASSERT_EQUALS("Half radius not calculated properly", 0.5, strategy.weightAt(adjX, ix, adjY, iy));
  }

  void testParabolicWeightingThrows()
  {
    ParabolicWeighting strategy;
    double distance = 0;
    TSM_ASSERT_THROWS("Should not be able to use the ParabolicWeighting like this.", strategy.weightAt(distance), std::runtime_error);
  }

  void testParabolicWeightingRectangular()
  {
    ParabolicWeighting strategy;

    int adjX = 2;
    int adjY = 2; 

    int ix = 2; int iy = 2;
    TSM_ASSERT_EQUALS("Top-Right not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = 2;
    TSM_ASSERT_EQUALS("Top-Left not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Right not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Left not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 0; iy = 0;
    TSM_ASSERT_EQUALS("Center not calculated properly", 5, strategy.weightAt(adjX, ix, adjY, iy));
  }


};


#endif /* MANTID_ALGORITHMS_WEIGHTINGSTRATEGYTEST_H_ */