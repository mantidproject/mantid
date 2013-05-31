#ifndef CONVERTSPECTRUMAXIS2TEST_H_
#define CONVERTSPECTRUMAXIS2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;

class ConvertSpectrumAxis2Test : public CxxTest::TestSuite
{
private:

  void do_algorithm_run(std::string target, std::string inputWS, std::string outputWS)
  {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","2");
    loader.setPropertyValue("SpectrumMax","3");
    //loader.setPropertyValue("Efixed","13.0");
    loader.execute();

    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Target",target) );

    TS_ASSERT_THROWS_NOTHING( conv.execute() );
    TS_ASSERT( conv.isExecuted() );
  }

public:

	void testName()
	{
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_EQUALS( conv.name(), "ConvertSpectrumAxis" );
	}

	void testVersion()
	{
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_EQUALS( conv.version(), 2 );
	}

  void testInit()
  {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_THROWS_NOTHING( conv.initialize() );
    TS_ASSERT( conv.isInitialized() );
  }

  void testTargetSignedTheta()
  {
    const std::string inputWS("inWS");
    const std::string outputSignedThetaAxisWS("outSignedThetaWS");

    do_algorithm_run("SignedTheta", inputWS, outputSignedThetaAxisWS);

    MatrixWorkspace_const_sptr outputSignedTheta;
    TS_ASSERT_THROWS_NOTHING( outputSignedTheta = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSignedThetaAxisWS) );
    
    // Check the signed theta axis
    const Axis* thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING( thetaAxis = outputSignedTheta->getAxis(1) );
    TS_ASSERT( thetaAxis->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis->unit()->caption(), "Scattering angle" );
    TS_ASSERT_EQUALS( thetaAxis->unit()->label(), "degrees" );

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputSignedThetaAxisWS);
  }

};

#endif /*CONVERTSPECTRUMAXIS2TEST_H_*/
