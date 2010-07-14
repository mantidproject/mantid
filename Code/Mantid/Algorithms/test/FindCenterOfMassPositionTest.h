#ifndef FINDCENTEROFMASSPOSITIONTEST_H_
#define FINDCENTEROFMASSPOSITIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/FindCenterOfMassPosition.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/TableRow.h"

#include "SANSInstrumentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class FindCenterOfMassPositionTest : public CxxTest::TestSuite
{
public:

  /*
   * Generate fake data for which we know what the result should be
   */
  void setUp()
  {
    inputWS = "sampledata";
    center_x = 25.5;
    center_y = 10.5;

    ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);

    // Generate sample data as a 2D Gaussian around the defined center
    for ( int ix=0; ix<SANSInstrumentCreationHelper::nBins; ix++ )
    {
      for ( int iy=0; iy<SANSInstrumentCreationHelper::nBins; iy++)
      {
        int i = ix*SANSInstrumentCreationHelper::nBins+iy+SANSInstrumentCreationHelper::nMonitors;
        MantidVec& X = ws->dataX(i);
        MantidVec& Y = ws->dataY(i);
        MantidVec& E = ws->dataE(i);
        X[0] = 1;
        X[1] = 2;
        double dx = (center_x-(double)ix);
        double dy = (center_y-(double)iy);
        Y[0] = exp(-(dx*dx+dy*dy));
        E[0] = 1;
        ws->getAxis(1)->spectraNo(i) = i;
      }
    }

  }

  void testName()
  {
    setUp();
    TS_ASSERT_EQUALS( center.name(), "FindCenterOfMassPosition" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( center.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( center.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( center.initialize() )
    TS_ASSERT( center.isInitialized() )
  }

  void testExec()
  {
    if (!center.isInitialized()) center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace",inputWS);
    center.setPropertyValue("OutputWorkspace",outputWS);
    center.setPropertyValue("NPixelX","30");
    center.setPropertyValue("NPixelY","30");

    TS_ASSERT_THROWS_NOTHING( center.execute() )
    TS_ASSERT( center.isExecuted() )

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS(table->rowCount(),2);
    TS_ASSERT_EQUALS(table->columnCount(),2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"X (m)");
    TS_ASSERT_DELTA(row.Double(1),25.5,0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"Y (m)");
    TS_ASSERT_DELTA(row.Double(1),10.5,0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  void testExecScatteredData()
  {
    if (!center.isInitialized()) center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace",inputWS);
    center.setPropertyValue("OutputWorkspace",outputWS);
    center.setPropertyValue("NPixelX","30");
    center.setPropertyValue("NPixelY","30");
    center.setPropertyValue("DirectBeam","0");
    center.setPropertyValue("BeamRadius", "1.5");

    TS_ASSERT_THROWS_NOTHING( center.execute() )
    TS_ASSERT( center.isExecuted() )

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS(table->rowCount(),2);
    TS_ASSERT_EQUALS(table->columnCount(),2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"X (m)");
    TS_ASSERT_DELTA(row.Double(1),25.5,0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"Y (m)");
    TS_ASSERT_DELTA(row.Double(1),10.5,0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  /*
   * Test that will load an actual data file and perform the center of mass
   * calculation. This test takes a longer time to execute so we won't include
   * it in the set of unit tests.
   */
  void emptyCell()
  {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/SANS2D/BioSANS_exp61_scan0002_0001_emptycell.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    if (!center.isInitialized()) center.initialize();

    TS_ASSERT_THROWS_NOTHING( center.setPropertyValue("InputWorkspace",inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( center.setPropertyValue("OutputWorkspace",outputWS) )
    center.setPropertyValue("NPixelX","192");
    center.setPropertyValue("NPixelY","192");

    TS_ASSERT_THROWS_NOTHING( center.execute() )
    TS_ASSERT( center.isExecuted() )
    
    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
    boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS(table->rowCount(),2);
    TS_ASSERT_EQUALS(table->columnCount(),2);

    // Check that the position is the same as obtained with the HFIR code
    // to within 0.3 pixel
    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"X (m)");
    TS_ASSERT_DELTA(row.Double(1),16.6038,0.3);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"Y (m)");
    TS_ASSERT_DELTA(row.Double(1),96.771,0.3);
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

private:
  Mantid::Algorithms::FindCenterOfMassPosition center;
  std::string inputWS;
  double center_x;
  double center_y;
  DataObjects::Workspace2D_sptr ws;

};

#endif /*FINDCENTEROFMASSPOSITIONTEST_H_*/
