#ifndef CALCULATEEFFICIENCYTEST_H_
#define CALCULATEEFFICIENCYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CalculateEfficiency.h"
#include "MantidAlgorithms/SANSSolidAngleCorrection.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class CalculateEfficiencyTest : public CxxTest::TestSuite
{
public:

  /*
   * Generate fake data for which we know what the result should be
   */
  void setUp()
  {
    inputWS = "sampledata";

    Mantid::DataObjects::Workspace2D_sptr ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);

    // Set up the X bin for the monitor channels
    for ( int i=0; i<SANSInstrumentCreationHelper::nMonitors; i++ )
    {
      Mantid::MantidVec& X = ws->dataX(i);
      Mantid::MantidVec& Y = ws->dataY(i);
      Mantid::MantidVec& E = ws->dataE(i);
      X[0] = 1;
      X[1] = 2;
    }

    for ( int ix=0; ix<SANSInstrumentCreationHelper::nBins; ix++ )
    {
      for ( int iy=0; iy<SANSInstrumentCreationHelper::nBins; iy++)
      {
        int i = ix*SANSInstrumentCreationHelper::nBins+iy+SANSInstrumentCreationHelper::nMonitors;
        Mantid::MantidVec& X = ws->dataX(i);
        Mantid::MantidVec& Y = ws->dataY(i);
        Mantid::MantidVec& E = ws->dataE(i);
        X[0] = 1;
        X[1] = 2;
        Y[0] = 2.0;
        E[0] = 1;
        ws->getAxis(1)->spectraNo(i) = i;
      }
    }
    // Change one of the bins so that it will be excluded for having a high signal
    Mantid::MantidVec& Y = ws->dataY(SANSInstrumentCreationHelper::nMonitors+5);
    Y[0] = 202.0;

  }

  void testName()
  {
    TS_ASSERT_EQUALS( correction.name(), "CalculateEfficiency" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( correction.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( correction.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( correction.initialize() )
    TS_ASSERT( correction.isInitialized() )
  }

  void testExec()
  {
    if (!correction.isInitialized()) correction.initialize();

    const std::string outputWS("result");
    correction.setPropertyValue("InputWorkspace",inputWS);
    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( correction.execute() )
    TS_ASSERT( correction.isExecuted() )

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING( ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS) );
    Mantid::DataObjects::Workspace2D_sptr ws2d_out = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    double tolerance(1e-03);
    TS_ASSERT_DELTA( ws2d_out->dataY(1+SANSInstrumentCreationHelper::nMonitors)[0], 0.9, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(15+SANSInstrumentCreationHelper::nMonitors)[0], 0.9, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(6+SANSInstrumentCreationHelper::nMonitors)[0], 0.9, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(5+SANSInstrumentCreationHelper::nMonitors)[0], 90.9, tolerance );

    TS_ASSERT_DELTA( ws2d_out->dataE(1+SANSInstrumentCreationHelper::nMonitors)[0], 0.4502, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataE(15+SANSInstrumentCreationHelper::nMonitors)[0], 0.4502, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataE(6+SANSInstrumentCreationHelper::nMonitors)[0], 0.4502, tolerance );

    // Check that pixels that were out of range were masked
    TS_ASSERT(!ws2d_out->getDetector(5+SANSInstrumentCreationHelper::nMonitors)->isMasked())
    TS_ASSERT(!ws2d_out->getDetector(1+SANSInstrumentCreationHelper::nMonitors)->isMasked())


    // Repeat the calculation by excluding high/low pixels

    TS_ASSERT_THROWS_NOTHING( correction.setProperty<double>("MinEfficiency",0.5) )
    TS_ASSERT_THROWS_NOTHING( correction.setProperty<double>("MaxEfficiency",1.50) )

    TS_ASSERT_THROWS_NOTHING( correction.execute() )
    TS_ASSERT( correction.isExecuted() )

    TS_ASSERT_THROWS_NOTHING( ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS) );
    ws2d_out = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    TS_ASSERT_DELTA( ws2d_out->dataX(1+SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataX(1+SANSInstrumentCreationHelper::nMonitors)[1], 2.0, tolerance );

    TS_ASSERT_DELTA( ws2d_out->dataY(1+SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(15+SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(6+SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance );

    TS_ASSERT_DELTA( ws2d_out->dataE(1+SANSInstrumentCreationHelper::nMonitors)[0], 0.5002, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataE(15+SANSInstrumentCreationHelper::nMonitors)[0], 0.5002, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataE(6+SANSInstrumentCreationHelper::nMonitors)[0], 0.5002, tolerance );

    // Check that pixels that were out of range were masked
    TS_ASSERT(ws2d_out->getDetector(5+SANSInstrumentCreationHelper::nMonitors)->isMasked())
    TS_ASSERT(!ws2d_out->getDetector(1+SANSInstrumentCreationHelper::nMonitors)->isMasked())

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  /*
   * Function that will validate results against known results found with
   * "standard" HFIR reduction package.
   */
  void validate()
  {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename","BioSANS_exp61_scan0004_0001.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    Mantid::DataHandling::MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace","wav");
    mover.setPropertyValue("ComponentName","detector1");
    // According to the instrument geometry, the center of the detector is located at N_pixel / 2 + 0.5
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    mover.setPropertyValue("X","0.409425");
    mover.setPropertyValue("Y","0.002575");
    mover.setPropertyValue("Z","6");
    mover.execute();

    // Solid angle correction
    Mantid::Algorithms::SANSSolidAngleCorrection sa_corr;
    const std::string sa_corrWS("sa_corrected");
    sa_corr.initialize();
    sa_corr.setPropertyValue("InputWorkspace", inputWS);
    sa_corr.setPropertyValue("OutputWorkspace", sa_corrWS);
    sa_corr.execute();

    if (!correction.isInitialized()) correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("InputWorkspace",sa_corrWS) )
    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( correction.setProperty<double>("MinEfficiency",0.5) )
    TS_ASSERT_THROWS_NOTHING( correction.setProperty<double>("MaxEfficiency",1.50) )

    correction.execute();

    TS_ASSERT( correction.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 36866 )

    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "Wavelength" )

    Mantid::API::Workspace_sptr ws_in;
    TS_ASSERT_THROWS_NOTHING( ws_in = Mantid::API::AnalysisDataService::Instance().retrieve(inputWS) );
    Mantid::DataObjects::Workspace2D_sptr ws2d_in = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_in);

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING( ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS) );
    Mantid::DataObjects::Workspace2D_sptr ws2d_out = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    // Number of monitors
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    // Get the coordinate of the detector pixel

    double tolerance(1e-03);
    TS_ASSERT_DELTA( ws2d_out->dataY(1+nmon)[0], 0.980083, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(193+nmon)[0], 1.23006, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataY(6+nmon)[0], 1.10898, tolerance );
    
    TS_ASSERT_DELTA( ws2d_out->dataE(1+nmon)[0], 0.0990047, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataE(193+nmon)[0], 0.110913, tolerance );
    TS_ASSERT_DELTA( ws2d_out->dataE(6+nmon)[0], 0.105261, tolerance );

    // Check that pixels that were out of range were masked
    TS_ASSERT(ws2d_out->getDetector(1826)->isMasked())
    TS_ASSERT(ws2d_out->getDetector(2014)->isMasked())
    TS_ASSERT(ws2d_out->getDetector(2015)->isMasked())

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);

  }

private:
  Mantid::Algorithms::CalculateEfficiency correction;
  std::string inputWS;
};

#endif /*Q1DTEST_H_*/
