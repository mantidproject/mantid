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
    MatrixWorkspace_sptr iws = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 10, 0, 1);
    MatrixWorkspace_sptr mws = WorkspaceCreationHelper::Create2DWorkspaceBinned(order+1, 15, 0.5, 0.1);

    size_t mwSize =  iws->readY(0).size();
    for (size_t i = 0; i < mwSize; ++i)
    {
      double val = static_cast<double>(i);
      iws->dataY(0)[i] = val *2;
    }

    alg.setProperty("WorkspaceToMatch", mws);
    alg.setProperty("WorkspaceToInterpolate", iws);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );

    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    const auto & yVals = outputWorkspace->readY(0);
    const auto & yDeriv = outputWorkspace->readY(1);
    const auto & yDeriv2 = outputWorkspace->readY(2);

    double count =1;
    for(size_t i = 0; i < yVals.size(); ++i)
    {
      TS_ASSERT_DELTA(yVals[i], count*0.1, 1e-15);
      TS_ASSERT_EQUALS(yDeriv[i], 2);
      TS_ASSERT_EQUALS(yDeriv2[i], 0);
      count+=2;
    }
  }

};


#endif /* MANTID_CURVEFITTING_SPLINEINTERPOLATIONTEST_H_ */
