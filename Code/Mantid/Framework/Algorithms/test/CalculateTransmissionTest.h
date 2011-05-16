#ifndef CALCULATETRANSMISSIONTEST_H_
#define CALCULATETRANSMISSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidCurveFitting/Linear.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;
using Mantid::API::MatrixWorkspace;

class CalculateTransmissionTest : public CxxTest::TestSuite
{
public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateTransmissionTest *createSuite() { return new CalculateTransmissionTest(); }
  static void destroySuite( CalculateTransmissionTest *suite ) { delete suite; }


  void testBasics()
  {
    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_EQUALS( trans.name(), "CalculateTransmission" );
    TS_ASSERT_EQUALS( trans.version(), 1 );
    TS_ASSERT_EQUALS( trans.category(), "SANS" );
  }

  void testFittedUnfitted()
  {

    Mantid::API::MatrixWorkspace_sptr inputWS =
      WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1,50,true);
    inputWS->getAxis(0)->unit() = 
      Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_THROWS_NOTHING( trans.initialize() );
    TS_ASSERT( trans.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( trans.setProperty("SampleRunWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("DirectRunWorkspace",inputWS) )
    std::string outputWS("CalculateTransmissionTest_outputWS");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setProperty("OutputUnfittedData", true) )

    TS_ASSERT_THROWS_NOTHING( trans.execute() );
    TS_ASSERT( trans.isExecuted() );

    Mantid::API::MatrixWorkspace_const_sptr fitted, unfitted;
    TS_ASSERT_THROWS_NOTHING(fitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    TS_ASSERT_THROWS_NOTHING(unfitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(outputWS+"_unfitted")) );
        
    const Mantid::MantidVec &fit = fitted->readY(0), &unfit = unfitted->readY(0);
    TS_ASSERT_EQUALS(fit.size(), unfit.size())
    for (unsigned int i = 0; i < fit.size(); ++i)
    {
      // Should all be 1 because I used the same workspace twice as the input
      TS_ASSERT_DELTA( fit[i], 1.0, 0.0005 )
      // a linear fit thorugh all 1s should result in all 1s
      TS_ASSERT_DELTA( fit[i], unfit[i], 0.0005 )
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS+"_unfitted");
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
  ///this tests where the output ranges is greater than the input range
  void testExtrapolationFit()
  {
    CalculateTransmission trans;
    trans.initialize();
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setPropertyValue("OutputWorkspace","CalculateTransmissionTest_extra");
    trans.setProperty("IncidentBeamMonitor",1);
    trans.setProperty("TransmissionMonitor",2);
    trans.setProperty("RebinParams","0.5, 0.1, 14");

    TS_ASSERT_THROWS_NOTHING( trans.execute() );
    TS_ASSERT( trans.isExecuted() );

    Mantid::API::MatrixWorkspace_const_sptr extra = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve("CalculateTransmissionTest_extra"));

    //these values were dervived from the debugger when exprolation was first added and are believed to be correct on that basis
    TS_ASSERT_DELTA( extra->readY(0)[0], 0.2, 0.8937 )
    TS_ASSERT_DELTA( extra->readY(0)[8], 0.2, 0.8801 )
    TS_ASSERT_DELTA( extra->readY(0)[18], 0.2, 0.8634 )
    TS_ASSERT_DELTA( extra->readY(0)[33], 0.2, 0.8390 )
    TS_ASSERT_DELTA( extra->readY(0)[54], 0.2, 0.8059 )
    TS_ASSERT_DELTA( extra->readY(0).back(), 0.2, 0.6914 )

    Mantid::API::AnalysisDataService::Instance().remove("CalculateTransmissionTest_extra");
  }

  ///fitting with log or linear should give similar results
  void testLogLin()
  {
    CalculateTransmission trans;
    trans.initialize();
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setPropertyValue("OutputWorkspace","CalculateTransmissionTest_log");
    trans.setProperty("IncidentBeamMonitor",1);
    trans.setProperty("TransmissionMonitor",2);
    trans.setProperty("RebinParams","0.8, 0.1, 8");
    TS_ASSERT_THROWS_NOTHING( trans.execute() );
    TS_ASSERT( trans.isExecuted() );
    
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setProperty("FitMethod","Linear");
    trans.setPropertyValue("OutputWorkspace","CalculateTransmissionTest_linear");
    TS_ASSERT_THROWS_NOTHING( trans.execute() );
    TS_ASSERT( trans.isExecuted() );
    Mantid::API::MatrixWorkspace_const_sptr logged, lineared;
    TS_ASSERT_THROWS_NOTHING(logged = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve("CalculateTransmissionTest_log")) );
    TS_ASSERT_THROWS_NOTHING(lineared = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve("CalculateTransmissionTest_linear")) );

    const Mantid::MantidVec &log = logged->readY(0), &linear = lineared->readY(0);

    TS_ASSERT_EQUALS(log.size(), linear.size())
    for (unsigned int i = 0; i < linear.size(); ++i)
    {
      //these are not expected to match exactly but, for sensible data, they should be close
      TS_ASSERT_DELTA( log[i]/linear[i], 1.0, 0.02 )
    }

    Mantid::API::AnalysisDataService::Instance().remove("CalculateTransmissionTest_log");
    Mantid::API::AnalysisDataService::Instance().remove("CalculateTransmissionTest_linear");
  }

  ///stops the construtor below from being run automatically for all tests
  static CalculateTransmissionTest *createSuite() { return new CalculateTransmissionTest(); }
  static void destroySuite(CalculateTransmissionTest *suite) { delete suite; }

  CalculateTransmissionTest() :
    m_dirWS("CalculateTransmissionTest_direct"), m_transWS("CalculateTransmissionTest_trans")
  {
    loadSampleLOQMonitors();
  }

  ~CalculateTransmissionTest()
  {
    Mantid::API::AnalysisDataService::Instance().remove(m_dirWS);
    Mantid::API::AnalysisDataService::Instance().remove(m_transWS);
  }

  ///Load and convert some monitor spectra to create some non-trival input data
  void loadSampleLOQMonitors()
  {
    //load a couple of real montior spectra
    std::string wkspName("LOQ48097");
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48097.raw");
    loader.setPropertyValue("OutputWorkspace",wkspName);
    loader.setProperty("SpectrumMin",1);
    loader.setProperty("SpectrumMax",2);
    loader.execute();
    //convert it to wavelength
    ConvertUnits unis;
    unis.initialize();
    unis.setPropertyValue("InputWorkspace",wkspName);
    unis.setPropertyValue("OutputWorkspace",wkspName);
    unis.setProperty("Target","Wavelength");
    unis.execute();
    //crop off prompt spikes
    Rebin crop;
    crop.initialize();
    crop.setPropertyValue("InputWorkspace",wkspName);
    crop.setPropertyValue("OutputWorkspace", m_dirWS);
    crop.setProperty("Params","6, 0.01, 7.5");
    crop.execute();
    crop.setPropertyValue("InputWorkspace",wkspName);
    crop.setPropertyValue("OutputWorkspace", m_transWS);
    crop.setProperty("Params","7.5, 0.01, 9");
    crop.execute();

    Mantid::API::AnalysisDataService::Instance().remove(wkspName);

    Mantid::API::MatrixWorkspace_sptr dir =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(m_dirWS)), source =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(m_transWS));
    Mantid::MantidVec & Xfiddle0 = dir->dataX(0), & Xsource = source->dataX(0);
    Mantid::MantidVec & Xfiddle1 = dir->dataX(1);
    for (unsigned int i = 0; i < Xfiddle0.size(); ++i)
    {
      Xfiddle0[i] = Xsource[i];
      Xfiddle1[i] = Xsource[i];
    }
  }

  private:
    ///these are the names of some sample data workspaces
    std::string m_dirWS, m_transWS;
};

#endif /*CALCULATETRANSMISSIONTEST_H_*/
