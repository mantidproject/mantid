#ifndef CONVERTSPECTRUMAXISTEST_H_
#define CONVERTSPECTRUMAXISTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/ConvertSpectrumAxis.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;

class ConvertSpectrumAxisTest : public CxxTest::TestSuite
{
private:

  void do_algorithm_run(std::string target, std::string inputWS, std::string outputWS)
  {
    Mantid::Algorithms::ConvertSpectrumAxis conv;
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
    Mantid::Algorithms::ConvertSpectrumAxis conv;
    TS_ASSERT_EQUALS( conv.name(), "ConvertSpectrumAxis" );
	}

	void testVersion()
	{
    Mantid::Algorithms::ConvertSpectrumAxis conv;
    TS_ASSERT_EQUALS( conv.version(), 1 );
	}

  void testInit()
  {
    Mantid::Algorithms::ConvertSpectrumAxis conv;
    TS_ASSERT_THROWS_NOTHING( conv.initialize() );
    TS_ASSERT( conv.isInitialized() );
  }

  void testTargetTheta()
  {
    const std::string inputWS("inWS");
    const std::string outputWS("outWS");
   
    do_algorithm_run("theta", inputWS, outputWS);
    
    MatrixWorkspace_const_sptr input,output;
    TS_ASSERT_THROWS_NOTHING( input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS) );
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );
    
    // Should now have a numeric axis up the side, with units of angle
    const Axis* thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING( thetaAxis = output->getAxis(1) );
    TS_ASSERT( thetaAxis->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis->unit()->caption(), "Scattering angle" );
    TS_ASSERT_EQUALS( thetaAxis->unit()->label(), "degrees" );
    TS_ASSERT_DELTA( (*thetaAxis)(0), 6.0883, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis)(1), 180.0, 0.0001 );
    // Check axis is correct length
    TS_ASSERT_THROWS( (*thetaAxis)(2), Mantid::Kernel::Exception::IndexError );

    // Data should be swapped over
    TS_ASSERT_EQUALS( input->readX(0), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(1) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(0) );

    //Clean up
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
  }

  void testTargetSignedTheta()
  {
    const std::string inputWS("inWS");
    const std::string outputSignedThetaAxisWS("outSignedThetaWS");

    do_algorithm_run("signed_theta", inputWS, outputSignedThetaAxisWS);

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

  void testEfixed()
  {
    Mantid::Algorithms::ConvertSpectrumAxis conv;
    conv.initialize();

    const std::string inputWS("inWS");
    const std::string outputWS("outWS");

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","IRS26173.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","12");
    loader.setPropertyValue("SpectrumMax","13");
    loader.execute();

    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Target","DeltaE") );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("EMode","Indirect") );
    conv.setRethrows(true);
    TS_ASSERT_THROWS( conv.execute(), std::logic_error );

    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Efixed","1.845") );
    TS_ASSERT_THROWS_NOTHING( conv.execute() );
    TS_ASSERT( conv.isExecuted() );

    MatrixWorkspace_const_sptr input,output;
    TS_ASSERT_THROWS_NOTHING( input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS) );
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );

    // Should now have a numeric axis up the side, with units of angle
    const Axis* thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING( thetaAxis = output->getAxis(1) );
    TS_ASSERT( thetaAxis->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis->unit()->caption(), "Energy transfer" );
    TS_ASSERT_EQUALS( thetaAxis->unit()->label(), "meV" );

    TS_ASSERT_DELTA( (*thetaAxis)(0), 0.00311225, 1e-08 );
    TS_ASSERT_DELTA( (*thetaAxis)(1), 0.00311225, 1e-08 );
    // Check axis is correct length
    TS_ASSERT_THROWS( (*thetaAxis)(2), Mantid::Kernel::Exception::IndexError );

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
  }
};

#endif /*CONVERTSPECTRUMAXISTEST_H_*/
