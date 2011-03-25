#ifndef CALCULATETRANSMISSIONTEST_H_
#define CALCULATETRANSMISSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidCurveFitting/Linear.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::API::MatrixWorkspace;

class CalculateTransmissionTest : public CxxTest::TestSuite
{
public:
  void testBasics()
  {
    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_EQUALS( trans.name(), "CalculateTransmission" );
    TS_ASSERT_EQUALS( trans.version(), 1 );
    TS_ASSERT_EQUALS( trans.category(), "SANS" );
  }

  void testExec()
  {

    Mantid::API::MatrixWorkspace_sptr inputWS =
      WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1,50,true);
    inputWS->getAxis(0)->unit() = 
      Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_THROWS_NOTHING( trans.initialize() );
    TS_ASSERT( trans.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( trans.setProperty("SampleRunWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("DirectRunWorkspace",inputWS) );
    std::string outputWS("CalculateTransmissionTest_outputWS");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) );

    TS_ASSERT_THROWS_NOTHING( trans.execute() );
    TS_ASSERT( trans.isExecuted() );
    
    Mantid::API::MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    const Mantid::MantidVec &Y = output->readY(0);
    // Should all be 1 because I used the same workspace twice as the input
    for (unsigned int i = 0; i < Y.size(); ++i)
    {
      TS_ASSERT_DELTA( Y[i], 1.0, 0.005 )
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testSingleBin()
  {
    // Create an test workspace with a single wavelength bin and test that
    // the algorithm completes.

    const std::string inputWS = "sampletransdata";

    Mantid::DataObjects::Workspace2D_sptr ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, ws);

    const std::string emptyWS("directbeam_ws");
    Mantid::DataObjects::Workspace2D_sptr empty_ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(emptyWS);

    // According to this detector geometry, Monitor #1 is spectrum 0, and Monitor #2 is spectrum 1.
    empty_ws->dataY(0)[0] = 10.0;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(emptyWS, empty_ws);

    TS_ASSERT_EQUALS( ws->dataY(0).size(), 1 )

    Mantid::Algorithms::CalculateTransmission trans;
    TS_ASSERT_THROWS_NOTHING( trans.initialize() );

    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("SampleRunWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("DirectRunWorkspace",emptyWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("IncidentBeamMonitor",1) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("TransmissionMonitor",2) )
    std::string outputWS("CalculateTransmissionTest_outputWS2");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) )

    trans.execute();
    TS_ASSERT( trans.isExecuted() )

    Mantid::API::MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( output->readY(0)[0], 5.0, 0.005 )

    // If we reverse the monitors, we should invert the output
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("IncidentBeamMonitor",2) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("TransmissionMonitor",1) )
    trans.execute();
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( output->readY(0)[0], 0.2, 0.005 )

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(emptyWS);
  }

};

#endif /*CALCULATETRANSMISSIONTEST_H_*/
