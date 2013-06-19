#ifndef CONVERTSPECTRUMAXIS2TEST_H_
#define CONVERTSPECTRUMAXIS2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::API;

class ConvertSpectrumAxis2Test : public CxxTest::TestSuite
{
private:

  void do_algorithm_run(std::string target, std::string inputWS, std::string outputWS, bool startYNegative = false)
  {
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 1, false, startYNegative);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);
 
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;

    //conv.setPropertyValue("EFixed", "10.0");

    conv.initialize();
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Target",target) );

    TS_ASSERT_THROWS_NOTHING( conv.execute() );
    TS_ASSERT( conv.isExecuted() );
  }
   
public:

	void estName()
	{
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_EQUALS( conv.name(), "ConvertSpectrumAxis" );
	}

	void estVersion()
	{
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_EQUALS( conv.version(), 2 );
	}

  void estInit()
  {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_THROWS_NOTHING( conv.initialize() );
    TS_ASSERT( conv.isInitialized() );
  }

  void estTargetSignedTheta() // Old version of alg did not follow the naming convention for units. Keep till the old version is deprecated.
  {
    const std::string inputWS("inWS");
    const std::string outputSignedThetaAxisWS("outSignedThetaWS");
    const std::string inputWS2("inWS2");
    const std::string outputSignedThetaAxisWS2("outSignedThetaWS2");
        
    do_algorithm_run("signed_theta", inputWS, outputSignedThetaAxisWS, true);
    do_algorithm_run("SignedTheta", inputWS2, outputSignedThetaAxisWS2, true);

    MatrixWorkspace_const_sptr outputSignedTheta,outputSignedTheta2;
    TS_ASSERT_THROWS_NOTHING( outputSignedTheta = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSignedThetaAxisWS) );
    TS_ASSERT_THROWS_NOTHING( outputSignedTheta2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSignedThetaAxisWS2) );
    
    // Check the signed theta axis
    const Axis* thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING( thetaAxis = outputSignedTheta->getAxis(1) );
    TS_ASSERT( thetaAxis->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis->unit()->caption(), "Scattering angle" );
    TS_ASSERT_EQUALS( thetaAxis->unit()->label(), "degrees" );
    TS_ASSERT_DELTA( (*thetaAxis)(0), -1.1458, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis)(1), 0.0000, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis)(2), 1.1458, 0.0001 );

    // Check axis is correct length
    TS_ASSERT_THROWS( (*thetaAxis)(3), Mantid::Kernel::Exception::IndexError );

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputSignedThetaAxisWS);
    
    // Check the signed theta axis
    const Axis* thetaAxis2 = 0;
    TS_ASSERT_THROWS_NOTHING( thetaAxis2 = outputSignedTheta2->getAxis(1) );
    TS_ASSERT( thetaAxis2->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis2->unit()->caption(), "Scattering angle" );
    TS_ASSERT_EQUALS( thetaAxis2->unit()->label(), "degrees" );
    TS_ASSERT_DELTA( (*thetaAxis2)(0), -1.1458, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis2)(1), 0.0000, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis2)(2), 1.1458, 0.0001 );

    // Check axis is correct length
    TS_ASSERT_THROWS( (*thetaAxis2)(3), Mantid::Kernel::Exception::IndexError );

    AnalysisDataService::Instance().remove(inputWS2);
    AnalysisDataService::Instance().remove(outputSignedThetaAxisWS2);
  }

  void estTargetTheta() // Compatible with old version of alg. Keep till the old version is deprecated.
  {
    const std::string inputWS("inWS");
    const std::string outputWS("outWS");
    //const std::string inputWS2("inWS2");
    //const std::string outputWS2("outWS2");
   
    do_algorithm_run("theta", inputWS, outputWS);
    //do_algorithm_run("Theta", inputWS2, outputWS2);
    
    MatrixWorkspace_const_sptr input,output,input2,output2;
    TS_ASSERT_THROWS_NOTHING( input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS) );
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );
    //TS_ASSERT_THROWS_NOTHING( input2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWS2) );
    //TS_ASSERT_THROWS_NOTHING( output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS2) );
    
    // Should now have a numeric axis up the side, with units of angle
    const Axis* thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING( thetaAxis = output->getAxis(1) );
    TS_ASSERT( thetaAxis->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis->unit()->caption(), "Scattering angle" );
    TS_ASSERT_EQUALS( thetaAxis->unit()->label(), "degrees" );
    TS_ASSERT_DELTA( (*thetaAxis)(0), 1.1458, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis)(1), 0.0000, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis)(2), 1.1458, 0.0001 );
    // Check axis is correct length
    TS_ASSERT_THROWS( (*thetaAxis)(3), Mantid::Kernel::Exception::IndexError );

    // Data should be swapped over
    TS_ASSERT_EQUALS( input->readX(0), output->readX(2) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(2) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(2) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(1) );
    TS_ASSERT_EQUALS( input->readX(2), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(2), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(2), output->readE(0) );
    
    //const Axis* thetaAxis2 = 0;
    //TS_ASSERT_THROWS_NOTHING( thetaAxis2 = output2->getAxis(1) );
    //TS_ASSERT( thetaAxis2->isNumeric() );
    //TS_ASSERT_EQUALS( thetaAxis2->unit()->caption(), "Scattering angle" );
    //TS_ASSERT_EQUALS( thetaAxis2->unit()->label(), "degrees" );
    //TS_ASSERT_DELTA( (*thetaAxis2)(0), 0.0000, 0.0001 );
    //TS_ASSERT_DELTA( (*thetaAxis2)(1), 1.1458, 0.0001 );
    //TS_ASSERT_DELTA( (*thetaAxis2)(2), 2.2906, 0.0001 );
    //// Check axis is correct length
    //TS_ASSERT_THROWS( (*thetaAxis2)(3), Mantid::Kernel::Exception::IndexError );

    //// Data should be swapped over
    //TS_ASSERT_EQUALS( input2->readX(0), output2->readX(2) );
    //TS_ASSERT_EQUALS( input2->readY(0), output2->readY(2) );
    //TS_ASSERT_EQUALS( input2->readE(0), output2->readE(2) );
    //TS_ASSERT_EQUALS( input2->readX(1), output2->readX(1) );
    //TS_ASSERT_EQUALS( input2->readY(1), output2->readY(1) );
    //TS_ASSERT_EQUALS( input2->readE(1), output2->readE(1) );
    //TS_ASSERT_EQUALS( input2->readX(2), output2->readX(0) );
    //TS_ASSERT_EQUALS( input2->readY(2), output2->readY(0) );
    //TS_ASSERT_EQUALS( input2->readE(2), output2->readE(0) );

    //Clean up
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
    //AnalysisDataService::Instance().remove(inputWS2);
    //AnalysisDataService::Instance().remove(outputWS2);
  }

  void est_Target_ElasticQ_Throws_When_No_Efixed_Provided_And_Not_In_Workspace()
  {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
    const std::string target("ElasticQ");
    
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 1, false, true);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);
 
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    conv.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Target",target) );

    TS_ASSERT_THROWS( conv.execute(), std::invalid_argument );
    TS_ASSERT( !conv.isExecuted() );
   }

  void est_Target_ElasticQ_Uses_Algorithm_EFixed_Value_If_Provided()
  {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
    const std::string target("ElasticQ");
    
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 1, false, true);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);
 
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    conv.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Target",target) );

    //TS_ASSERT( != EMPTY_DBL);
  }

  void estTargetElasticQ() // The new version of this alg follows the standard for the naming of units.
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
    
    double eFixed;
    double constant = (4 * M_PI * sqrt(2 * Mantid::PhysicalConstants::NeutronMass * eFixed ))/Mantid::PhysicalConstants::h;

    TS_ASSERT_DELTA( (*qAxis)(0), 0.0000, 0.0001 );
    TS_ASSERT_DELTA( (*qAxis)(1), constant * 0.0200, 0.0001 );
    TS_ASSERT_DELTA( (*qAxis)(2), constant * 0.0400, 0.0001 );

    // Check axis is correct length
    TS_ASSERT_THROWS( (*qAxis)(2), Mantid::Kernel::Exception::IndexError );

    TS_ASSERT_EQUALS( input->readX(0), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(0) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(1) );
    TS_ASSERT_EQUALS( input->readX(2), output->readX(2) );
    TS_ASSERT_EQUALS( input->readY(2), output->readY(2) );
    TS_ASSERT_EQUALS( input->readE(2), output->readE(2) );

    //Clean up
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
    }

  void estTargetElasticQSquared() // The new version of this alg follows the standard for the naming of units.
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

    double eFixed;
    double constant = (32 * pow(M_PI, 2) * Mantid::PhysicalConstants::NeutronMass * eFixed )/pow(Mantid::PhysicalConstants::h, 2);
    
    TS_ASSERT_DELTA( (*q2Axis)(0), 0.0000, 0.0001 );
    TS_ASSERT_DELTA( (*q2Axis)(1), constant * 0.0004, 0.0001 );
    TS_ASSERT_DELTA( (*q2Axis)(2), constant * 0.0016, 0.0001 );
    
    // Check axis is correct length
    TS_ASSERT_THROWS( (*q2Axis)(2), Mantid::Kernel::Exception::IndexError );

    TS_ASSERT_EQUALS( input->readX(0), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(0) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(1) );
    TS_ASSERT_EQUALS( input->readX(2), output->readX(2) );
    TS_ASSERT_EQUALS( input->readY(2), output->readY(2) );
    TS_ASSERT_EQUALS( input->readE(2), output->readE(2) );

    //Clean up
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
    }
};

#endif /*CONVERTSPECTRUMAXIS2TEST_H_*/
