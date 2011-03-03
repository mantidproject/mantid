#ifndef LOADSPICE2DTEST_H
#define LOADSPICE2DTEST_H

//------------------------------------------------
// Includes
//------------------------------------------------

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include <Poco/Path.h>
#include <vector>

/**
 * Test HFIR SANS Spice loader
 * TODO: check that an exception is thrown when the geometry file doesn't define all monitors
 */
class LoadSpice2dTest : public CxxTest::TestSuite
{
public:
  static LoadSpice2dTest *createSuite() { return new LoadSpice2dTest(); }
  static void destroySuite(LoadSpice2dTest *suite) { delete suite; }

  LoadSpice2dTest()
  {
     inputFile = "BioSANS_exp61_scan0004_0001.xml";
  }
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( spice2d.initialize());
    TS_ASSERT( spice2d.isInitialized() );
  }

  void testExec()
  {
    if ( !spice2d.isInitialized() ) spice2d.initialize();

    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(spice2d.execute(), std::runtime_error);

    //Set the file name
    spice2d.setPropertyValue("Filename", inputFile);
    
    std::string outputSpace = "outws";
    //Set an output workspace
    spice2d.setPropertyValue("OutputWorkspace", outputSpace);
    
    //check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = spice2d.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now throw nothing
    TS_ASSERT_THROWS_NOTHING( spice2d.execute() );
    TS_ASSERT( spice2d.isExecuted() );

   //Now need to test the resultant workspace, first retrieve it
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );
    Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);

    // We have 192*192 + 2 channels, for the PSD + timer + monitor
    TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 36864 + Mantid::DataHandling::LoadSpice2D::nMonitors );

    //Test the size of the data vectors
    TS_ASSERT_EQUALS( (ws2d->dataX(0).size()), 2);
    TS_ASSERT_EQUALS( (ws2d->dataY(0).size()), 1);
    TS_ASSERT_EQUALS( (ws2d->dataE(0).size()), 1);


    double tolerance(1e-04);
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA( ws2d->dataX(0+nmon)[0], 5.93, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(2+nmon)[0], 5.93, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(192+nmon)[0], 5.93, tolerance );

    TS_ASSERT_DELTA( ws2d->dataY(0+nmon)[0], 318.0, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(2+nmon)[0], 109.0, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(192+nmon)[0], 390.0, tolerance );

    TS_ASSERT_DELTA( ws2d->dataE(0+nmon)[0], 17.8325, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(2+nmon)[0], 10.4403, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(192+nmon)[0], 19.7484, tolerance );

    // check monitor
    TS_ASSERT_DELTA( ws2d->dataY(0)[0], 29205906.0, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[0], 5404.2488, tolerance );

    // check timer
    TS_ASSERT_DELTA( ws2d->dataY(1)[0], 3600.0, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(1)[0], 0.0, tolerance );




    // Check instrument
    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<Mantid::Geometry::IInstrument> i = ws2d->getInstrument();
    boost::shared_ptr<Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS( i->getName(), "GPSANS");
    TS_ASSERT_EQUALS( source->getName(), "source");

    // Check parameters for sample aperture
    boost::shared_ptr<Mantid::Geometry::IComponent> sample_aperture = i->getComponentByName("sample_aperture");
    TS_ASSERT_EQUALS( sample_aperture->getNumberParameter("Size")[0], 14.0);

    // Check parameter map access
    const Mantid::Geometry::ParameterMap *m_paraMap = &(ws2d->instrumentParameters());

    // Check that we can get a parameter
    boost::shared_ptr<Mantid::Geometry::Parameter> sample_aperture_size = m_paraMap->get(sample_aperture.get(), "Size");
    TS_ASSERT_EQUALS( sample_aperture_size->type(), "double");
    TS_ASSERT_EQUALS( sample_aperture_size->value<double>(), 14.0);

    // Check that we can modify a parameter
    Mantid::Geometry::ParameterMap & pmap_nonconst = ws2d->instrumentParameters();
    pmap_nonconst.addDouble(sample_aperture.get(), "Size", 15.0);
    sample_aperture_size = m_paraMap->get(sample_aperture.get(), "Size");
    TS_ASSERT_EQUALS( sample_aperture_size->value<double>(), 15.0);

    boost::shared_ptr<Mantid::Geometry::Parameter> d = m_paraMap->get(i.get(), "sample-detector-distance");
    TS_ASSERT_EQUALS( d->type(), "double");
    TS_ASSERT_EQUALS( d->value<double>(), 6000.0);

    // Check detector position
    TS_ASSERT_EQUALS( i->getComponentByName("detector1")->getPos().Z(), 6.0);
  }


  void testExecChooseWavelength()
  {
    Mantid::DataHandling::LoadSpice2D spice2d;

    if ( !spice2d.isInitialized() ) spice2d.initialize();

    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(spice2d.execute(), std::runtime_error);

    //Set the file name
    spice2d.setPropertyValue("Filename", inputFile);

    std::string outputSpace = "outws";
    //Set an output workspace
    spice2d.setPropertyValue("OutputWorkspace", outputSpace);
    spice2d.setPropertyValue("Wavelength", "5.0");
    spice2d.setPropertyValue("WavelengthSpread", "1.0");


    //Should now throw nothing
    TS_ASSERT_THROWS_NOTHING( spice2d.execute() );
    TS_ASSERT( spice2d.isExecuted() );

   //Now need to test the resultant workspace, first retrieve it
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );
    Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);

    //Test the size of the data vectors
    TS_ASSERT_EQUALS( (ws2d->dataX(0).size()), 2);
    TS_ASSERT_EQUALS( (ws2d->dataY(0).size()), 1);
    TS_ASSERT_EQUALS( (ws2d->dataE(0).size()), 1);


    double tolerance(1e-04);
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA( ws2d->dataX(0+nmon)[0], 4.5, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(2+nmon)[0], 4.5, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(192+nmon)[0], 4.5, tolerance );
  }
private:
  std::string inputFile;
  Mantid::DataHandling::LoadSpice2D spice2d;

};
#endif
