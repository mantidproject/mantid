#ifndef CALCULATETRANSMISSIONTEST_H_
#define CALCULATETRANSMISSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidCurveFitting/Linear.h"
#include "SANSInstrumentCreationHelper.h"

using namespace Mantid::API;

class CalculateTransmissionTest : public CxxTest::TestSuite
{
public:
  CalculateTransmissionTest() : trans(), inputWS("LOQWS")
  {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","2");
    loader.setPropertyValue("SpectrumMax","4");
    loader.execute();

    Mantid::DataHandling::LoadInstrument inst;
    inst.initialize();
    inst.setPropertyValue("Workspace",inputWS);
    inst.setPropertyValue("Filename","../../../Instrument/LOQ_trans_Definition.xml");
    inst.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",inputWS);
    convert.setPropertyValue("OutputWorkspace",inputWS);
    convert.setPropertyValue("Target","Wavelength");
    convert.setPropertyValue("AlignBins","1");
    convert.execute();    
  }
  
  void testName()
  {
    TS_ASSERT_EQUALS( trans.name(), "CalculateTransmission" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( trans.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( trans.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( trans.initialize() )
    TS_ASSERT( trans.isInitialized() )	  
  }

  void testExec()
  {
    if ( !trans.isInitialized() ) trans.initialize();

    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("SampleRunWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("DirectRunWorkspace",inputWS) )
    std::string outputWS("outputWS");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( trans.execute() )
    TS_ASSERT( trans.isExecuted() )
    
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) )
    const Mantid::MantidVec &Y = output->readY(0);
    // Should all be 1 because I used the same workspace twice as the input
    for (unsigned int i = 0; i < Y.size(); ++i)
    {
      TS_ASSERT_DELTA( Y[i], 1.0, 0.005 )
    }
  }

  void testSingleBin()
  {
    // Create an test workspace with a single wavelength bin and test that
    // the algorithm completes.

    inputWS = "sampletransdata";

    DataObjects::Workspace2D_sptr ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, ws);

    const std::string emptyWS("directbeam_ws");
    DataObjects::Workspace2D_sptr empty_ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(emptyWS);

    // According to this detector geometry, Monitor #1 is spectrum 0, and Monitor #2 is spectrum 1.
    empty_ws->dataY(0)[0] = 10.0;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(emptyWS, empty_ws);

    TS_ASSERT_EQUALS( ws->dataY(0).size(), 1 )

    Mantid::Algorithms::CalculateTransmission trans;
    if ( !trans.isInitialized() ) trans.initialize();

    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("SampleRunWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("DirectRunWorkspace",emptyWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("IncidentBeamMonitor",1) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("TransmissionMonitor",2) )
    std::string outputWS("outputWS2");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) )

    trans.execute();
    TS_ASSERT( trans.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( output->readY(0)[0], 5.0, 0.005 )

    // If we reverse the monitors, we should invert the output
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("IncidentBeamMonitor",2) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("TransmissionMonitor",1) )
    trans.execute();
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( output->readY(0)[0], 0.2, 0.005 )

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(emptyWS);
  }

private:
  Mantid::Algorithms::CalculateTransmission trans;
  std::string inputWS;
};

#endif /*CALCULATETRANSMISSIONTEST_H_*/
