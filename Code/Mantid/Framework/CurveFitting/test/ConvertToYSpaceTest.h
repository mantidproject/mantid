#ifndef MANTID_CURVEFITTING_CONVERTTOYSPACETEST_H_
#define MANTID_CURVEFITTING_CONVERTTOYSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ConvertToYSpace.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::ConvertToYSpace;

class ConvertToYSpaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToYSpaceTest *createSuite() { return new ConvertToYSpaceTest(); }
  static void destroySuite( ConvertToYSpaceTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertToYSpace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  // --------------------------------- Success cases -----------------------------------
  
  void test_exec_with_TOF_input_gives_correct_X_values()
  {
    double x0(370.0),x1(371.0),dx(0.5); //chosen to give put us near the peak for this mass & spectrum
    auto testWS = ComptonProfileTestHelpers::createSingleSpectrumWorkspace(x0,x1,dx);

  }
  
  // --------------------------------- Failure cases -----------------------------------

  void test_Negative_Or_Zero_Mass_Throws_Error()
  {
    auto alg = createAlgorithm();

    // Zero
    TS_ASSERT_THROWS(alg->setProperty("Mass", 0.0), std::invalid_argument);
    // Negative
    TS_ASSERT_THROWS(alg->setProperty("Mass", -0.1), std::invalid_argument);
  }

  void test_Input_Workspace_Not_In_TOF_Throws_Error()
  {
    auto alg = createAlgorithm();
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace123(1,10);
    testWS->getAxis(0)->setUnit("Wavelength");

    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", testWS), std::invalid_argument);
  }

  void test_Input_Workspace_In_TOF_Without_Instrument_Throws_Error()
  {
    auto alg = createAlgorithm();
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace123(1,10);
    testWS->getAxis(0)->setUnit("TOF");

    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", testWS), std::invalid_argument);
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm()
  {
    Mantid::API::IAlgorithm_sptr alg = boost::make_shared<ConvertToYSpace>();
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("OutputWorkspace", "__UNUSED__");
    return alg;
  }

};


#endif /* MANTID_CURVEFITTING_CONVERTTOYSPACETEST_H_ */
