#ifndef CONVERTSPECTRUMAXIS2TEST_H_
#define CONVERTSPECTRUMAXIS2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel\UnitFactory.h"

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

  void testTargetSignedThetaOld() // Old version of alg did not follow the naming convention for units. Keep till the old version is deprecated.
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
    
  void testTargetSignedThetaNew() // The new version of this alg follows the standard for the naming of units.
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

  void testTargetThetaOld() // Old version of alg did not follow the naming convention for units. Keep till the old version is deprecated.
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

  void testTargetThetaNew() // The new version of this alg follows the standard for the naming of units.
  {
    const std::string inputWS("inWS");
    const std::string outputWS("outWS");
   
    do_algorithm_run("Theta", inputWS, outputWS);
    
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

  void testTargetElasticQ() // The new version of this alg follows the standard for the naming of units.
  {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
   
    do_algorithm_run("ElasticQ", inputWS, outputWS);
    
    MatrixWorkspace_const_sptr input,output;
    TS_ASSERT_THROWS_NOTHING( input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS) );
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );
       
    // Should now have a numeric axis up the side, with units of Q
    const Axis* qAxis = 0;
    TS_ASSERT_THROWS_NOTHING( qAxis = output->getAxis(1) );
    TS_ASSERT( qAxis->isNumeric() );
    TS_ASSERT_EQUALS( qAxis->unit()->unitID(), "MomentumTransfer");
    
    TS_ASSERT_DELTA( (*qAxis)(0), 0.0000, 0.0001 );

    //TS_ASSERT_EQUALS( qAxis->unit()->label(), Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer")->label() )
    // Check axis is correct length
    TS_ASSERT_THROWS( (*qAxis)(2), Mantid::Kernel::Exception::IndexError );

    TS_ASSERT_EQUALS( input->readX(0), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(0) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(1) );

    //Clean up
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
    }

  void testTargetElasticQSquared() // The new version of this alg follows the standard for the naming of units.
  {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
   
    do_algorithm_run("ElasticQSquared", inputWS, outputWS);
    
    MatrixWorkspace_const_sptr input,output;
    TS_ASSERT_THROWS_NOTHING( input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS) );
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );
       
    // Should now have a numeric axis up the side, with units of Q^2
    const Axis* q2Axis = 0;
    TS_ASSERT_THROWS_NOTHING( q2Axis = output->getAxis(1) );
    TS_ASSERT( q2Axis->isNumeric() );
    TS_ASSERT_EQUALS( q2Axis->unit()->unitID(), "QSquared");
    
    TS_ASSERT_DELTA( (*q2Axis)(0), 0.0000, 0.0001 );
    
    // double efixed = Mantid::Algorithms::ConvertSpectrumAxis2::getEfixed; ((32 * M_PI * M_PI * Mantid::Algorithms::ConvertSpectrumAxis2::getEfixed * Mantid::PhysicalConstants::NeutronMass)/(Mantid::PhysicalConstants::h * Mantid::PhysicalConstants::h))
    // TS_ASSERT_DELTA( (*q2Axis)(1), Mantid::Algorithms::ConvertSpectrumAxis2().m_elasticQSquared , 0.0001 );
    TS_ASSERT_DELTA( (*q2Axis)(1), ( 16 * pow(M_PI, 2.0)/pow(Mantid::Algorithms::ConvertSpectrumAxis2().m_wavelength, 2.0)) , 0.0001 )

    // Check axis is correct length
    TS_ASSERT_THROWS( (*q2Axis)(2), Mantid::Kernel::Exception::IndexError );

    TS_ASSERT_EQUALS( input->readX(0), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(0) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(1) );

    //Clean up
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
    }
};

#endif /*CONVERTSPECTRUMAXIS2TEST_H_*/
