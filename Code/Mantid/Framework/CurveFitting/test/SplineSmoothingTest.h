#ifndef MANTID_ALGORITHMS_SPLINETEST_H_
#define MANTID_ALGORITHMS_SPLINETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCurveFitting/SplineSmoothing.h"

using Mantid::CurveFitting::SplineSmoothing;

class SplineSmoothingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SplineSmoothingTest *createSuite() { return new SplineSmoothingTest(); }
  static void destroySuite( SplineSmoothingTest *suite ) { delete suite; }


  void test_Init()
  {
    SplineSmoothing alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    using namespace Mantid::API;

    // Name of the output workspace.
    std::string outWSName("SplineTest_OutputWS");

    //number of derivatives
    int order = 2;

    SplineSmoothing alg;

    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SplineSize", 200));
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Order", order));

    //create a binned workspace
    MatrixWorkspace_sptr inputWorkspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(order+1, 400, 0, 0.5);

    for (size_t i = 0; i <= inputWorkspace->readY(0).size(); ++i)
    {
      double val = static_cast<double>(i);
      inputWorkspace->dataY(0)[i] = val *2;
    }

    alg.setProperty("InputWorkspace", inputWorkspace);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );

    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    const auto & yVals = outputWorkspace->readY(0);

    for(size_t i = 0; i < yVals.size(); ++i)
    {
      TS_ASSERT_EQUALS(yVals[i], i* 2);
    }
  }
  
};


#endif /* MANTID_ALGORITHMS_SPLINETEST_H_ */
