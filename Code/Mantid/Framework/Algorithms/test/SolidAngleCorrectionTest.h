#ifndef SOLIDANGLECORRECTIONTEST_H_
#define SOLIDANGLECORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SolidAngleCorrection.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class SolidAngleCorrectionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( correction.name(), "SolidAngleCorrection" )
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
    // X = (16-192.0/2.0)*5.15/1000.0 = -0.412
    // Y = (95-192.0/2.0)*5.15/1000.0 = -0.00515
    mover.setPropertyValue("X","0.412");
    mover.setPropertyValue("Y","0.00515");
    mover.execute();

    if (!correction.isInitialized()) correction.initialize();

    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("InputWorkspace",inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( correction.execute() )

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

    // Pick a bin to test
    int i = 130;
    // Number of monitors
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    // Get the coordinate of the detector pixel
    double iy = (i-nmon)%192;
    double ix = (int)((i-nmon)/192);
    double r = sqrt(1.0+5.15*5.15/6000/6000*( (ix-16.0)*(ix-16.0) + (iy-95.0)*(iy-95.0) ));
    double corr = r*r*r;
    double ratio = ws2d_out->dataY(130+nmon)[0]/ws2d_in->dataY(130+nmon)[0];

    double tolerance(1e-03);
    TS_ASSERT_DELTA( ratio, corr, tolerance );
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);

  }

private:
  Mantid::Algorithms::SolidAngleCorrection correction;
  std::string inputWS;
};

#endif /*Q1DTEST_H_*/
