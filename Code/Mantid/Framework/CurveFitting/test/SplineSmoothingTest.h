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

  //Functor for generating data
  struct SplineFunc
  {
    double operator()(double x, int)
    {
      return std::sin(x);
    }
  };

  void test_Init()
  {
    SplineSmoothing alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void testExec()
  {
    using namespace Mantid::API;

    //number of derivatives and spectra
    const int order (2), spectra(2);

    //create a binned workspace
    MatrixWorkspace_sptr inputWorkspace = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 5, 0.02, false);

    //setup algorithm
    SplineSmoothing alg;
    runAlgorithm(alg, order, inputWorkspace);
    checkOutput(alg);
  }

  void testExecHistogramData()
  {
    using namespace Mantid::API;

    //number of derivatives and spectra
    const int order (2), spectra(1);

    //create a binned workspace
    MatrixWorkspace_sptr inputWorkspace = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 5, 0.02, true);

    SplineSmoothing alg;
    runAlgorithm(alg, order, inputWorkspace);
    checkOutput(alg);
  }
  
  void testExecMultipleHistograms()
  {
    using namespace Mantid::API;

    //number of derivatives and spectra
    const int order (2), spectra(3);

    //create a binned workspace
    MatrixWorkspace_sptr inputWorkspace = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(SplineFunc(), spectra, 0, 5, 0.02, true);

    SplineSmoothing alg;
    runAlgorithm(alg, order, inputWorkspace);
    checkOutput(alg);
  }

  void checkOutput(const SplineSmoothing& alg) const
  {
    using namespace Mantid::API;

    MatrixWorkspace_const_sptr ows = alg.getProperty("OutputWorkspace");
    WorkspaceGroup_const_sptr derivs = alg.getProperty("OutputWorkspaceDeriv");

    for (size_t i = 0; i < ows->getNumberHistograms(); ++i)
    {
      MatrixWorkspace_const_sptr derivsWs = boost::dynamic_pointer_cast<const MatrixWorkspace>(derivs->getItem(i));
      const auto & xs = ows->readX(i);
      const auto & ys = ows->readY(i);
      const auto & d1 = derivsWs->readY(0);
      const auto & d2 = derivsWs->readY(1);

      //check output for consistency
      for(size_t j = 0; j < ys.size(); ++j)
      {
        TS_ASSERT_DELTA(ys[j], std::sin(xs[j]), 1e-4);
        TS_ASSERT_DELTA(d1[j], std::cos(xs[j]), 1e-1);
        TS_ASSERT_DELTA(d2[j], -std::sin(xs[j]), 1e-1);
      }
    }
  }

  void runAlgorithm(SplineSmoothing& alg, int order, const Mantid::API::MatrixWorkspace_sptr& iws) const
  {
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Anon");
    alg.setPropertyValue("OutputWorkspaceDeriv", "AnonDerivs");

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Error", 0.05));
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DerivOrder", order));

    alg.setProperty("InputWorkspace", iws);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }

};


#endif /* MANTID_ALGORITHMS_SPLINETEST_H_ */
