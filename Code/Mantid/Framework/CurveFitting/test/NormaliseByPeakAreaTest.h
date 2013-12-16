#ifndef MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_
#define MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/NormaliseByPeakArea.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::NormaliseByPeakArea;

class NormaliseByPeakAreaTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByPeakAreaTest *createSuite() { return new NormaliseByPeakAreaTest(); }
  static void destroySuite( NormaliseByPeakAreaTest *suite ) { delete suite; }


  void test_Init()
  {
    NormaliseByPeakArea alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec_with_TOF_input_gives_correct_X_values()
  {
    using namespace Mantid::API;

    auto alg = createAlgorithm();
    double x0(50.0),x1(300.0),dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createSingleSpectrumWorkspace(x0,x1,dx, true,true);
    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS != 0)

    TS_ASSERT_EQUALS(testWS->getNumberHistograms(), outputWS->getNumberHistograms());

    // Test a few values
    const auto &outX = outputWS->readX(0);
    const auto &outY = outputWS->readY(0);
    const auto &outE = outputWS->readE(0);
    const size_t npts = outputWS->blocksize();
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm()
  {
    Mantid::API::IAlgorithm_sptr alg = boost::make_shared<NormaliseByPeakArea>();
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("OutputWorkspace", "__UNUSED__");
    return alg;
  }

};


#endif /* MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_ */
