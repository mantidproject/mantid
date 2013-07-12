#ifndef MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_
#define MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/SplineInterpolation.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::SplineInterpolation;

class SplineInterpolationTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SplineInterpolationTest *createSuite() { return new SplineInterpolationTest(); }
  static void destroySuite( SplineInterpolationTest *suite ) { delete suite; }


  void test_Init()
  {
    SplineInterpolation alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    using namespace Mantid::API;

    // Name of the output workspace.
    std::string outWSName("SplineInterpolationTest_OutputWS");
  
    SplineInterpolation alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    //number of derivatives
    int order = 2;

    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Order", 2));

    //create a binned workspaces
    MatrixWorkspace_sptr matchWorkspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 10, 0, 1);
    MatrixWorkspace_sptr interpolateWorkspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(order+1, 20, 0, 0.5);

    size_t mwSize =  matchWorkspace->readY(0).size();
    for (size_t i = 0; i < mwSize; ++i)
    {
      double val = static_cast<double>(i);
      matchWorkspace->dataY(0)[i] = val *2;
    }

    alg.setProperty("WorkspaceToMatch", matchWorkspace);
    alg.setProperty("WorkspaceToInterpolate", interpolateWorkspace);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );

    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    const auto & xVals = interpolateWorkspace->readX(0);
    const auto & yVals = outputWorkspace->readY(0);

    for(size_t i = 0; i < yVals.size(); ++i)
    {
      TS_ASSERT_EQUALS(yVals[i], xVals[i] * 2);
    }

  }

};


#endif /* MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_ */
