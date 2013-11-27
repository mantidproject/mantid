#ifndef MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CalculateGammaBackground.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::CalculateGammaBackground;

class CalculateGammaBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateGammaBackgroundTest *createSuite() { return new CalculateGammaBackgroundTest(); }
  static void destroySuite( CalculateGammaBackgroundTest *suite ) { delete suite; }

  //------------------------------------ Success cases ---------------------------------------
  void test_Input_With_Single_Mass_Gives_Expected_Output_Workspaces()
  {
    auto alg = createAlgorithm();
    alg->setRethrows(true);

    alg->setProperty("InputWorkspace",createTestWorkspaceWithFoilChanger());

    alg->setPropertyValue("Masses", "1.0079");
    alg->setPropertyValue("PeakAmplitudes", "2.9e-2");
    alg->setPropertyValue("PeakWidths", "4.29");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    using namespace Mantid::API;
    MatrixWorkspace_sptr backgroundWS = alg->getProperty("BackgroundWorkspace");
    MatrixWorkspace_sptr correctedWS = alg->getProperty("CorrectedWorkspace");
    TS_ASSERT(backgroundWS);
    TS_ASSERT(correctedWS);
    TS_ASSERT(backgroundWS != correctedWS);
  }


  //------------------------------------ Error cases ---------------------------------------

  void test_Peak_Information_Lists_Of_Zero_Length_Throw_An_Error()
  {
    auto alg = createAlgorithm();
    alg->setRethrows(true);

    alg->setProperty("InputWorkspace",createTestWorkspaceWithFoilChanger());

    // None set=all empty
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    // Just set masses
    alg->setPropertyValue("Masses", "1,2,3");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    // now amplitudes
    alg->setPropertyValue("PeakAmplitudes", "1,2,3");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    // now widths
    alg->setPropertyValue("PeakWidths", "1,2,3");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void test_Peak_Information_Lists_Of_Different_Lengths_Throws_Error()
  {
    auto alg = createAlgorithm();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace",createTestWorkspaceWithFoilChanger());

    alg->setProperty("Masses", "1,2,3");
    alg->setProperty("PeakAmplitudes", "1,2");
    alg->setProperty("PeakWidths", "1,2,3,4");
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_InputWorkspace_Without_FoilChanger_Component_Throws_Error()
  {

  }

private:

  Mantid::API::IAlgorithm_sptr createAlgorithm()
  {
    Mantid::API::IAlgorithm_sptr alg = boost::make_shared<CalculateGammaBackground>();
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("CorrectedWorkspace", "__UNUSED__");
    alg->setPropertyValue("BackgroundWorkspace", "__UNUSED__");
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspaceWithFoilChanger()
  {
    double x0(165.0),x1(166.0),dx(0.5);
    return ComptonProfileTestHelpers::createSingleSpectrumWorkspaceWithSingleMass(x0,x1,dx);
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspaceWithNoFoilChanger()
  {
    double x0(165.0),x1(166.0),dx(0.5);
    return ComptonProfileTestHelpers::createSingleSpectrumWorkspaceOfOnes(x0,x1,dx);

  }


};


#endif /* MANTID_ALGORITHMS_CalculateGammaBackgroundTEST_H_ */
