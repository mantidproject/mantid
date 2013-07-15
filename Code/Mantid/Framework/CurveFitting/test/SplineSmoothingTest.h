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

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SplineSize", 10));
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Order", order));

    //create a binned workspace
    MatrixWorkspace_sptr inputWorkspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(order+1, 20, 0, 1);

    size_t iwSize = inputWorkspace->readY(0).size();
    for (size_t i = 0; i < iwSize; ++i)
    {
      double val = static_cast<double>(i);
      inputWorkspace->dataY(0)[i] = val*2;
    }

    alg.setProperty("InputWorkspace", inputWorkspace);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );

    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    const auto & yVals = outputWorkspace->readY(0);
    const auto & yDeriv = outputWorkspace->readY(1);
    const auto & yDeriv2 = outputWorkspace->readY(2);

    for(size_t i = 0; i < yVals.size(); ++i)
    {
      TS_ASSERT_DELTA(yVals[i], i*2, 1e-15);
      TS_ASSERT_EQUALS(yDeriv[i], 2);
      TS_ASSERT_EQUALS(yDeriv2[i], 0);
    }
  }
  
};


#endif /* MANTID_ALGORITHMS_SPLINETEST_H_ */
