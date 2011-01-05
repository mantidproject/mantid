#ifndef ManagedRawFileWorkspace2DTEST_H_
#define ManagedRawFileWorkspace2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include <iostream>
#include "MantidKernel/ConfigService.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "Poco/Path.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class ManagedRawFileWorkspace2DTest : public CxxTest::TestSuite
{
public:
  ManagedRawFileWorkspace2DTest()
  {
    Workspace = new ManagedRawFileWorkspace2D("../../../../../Test/AutoTestData/HET15869.raw",2);
  }

  ~ManagedRawFileWorkspace2DTest()
  {
    delete Workspace;
  }

  void testSetFile()
  {
    TS_ASSERT_EQUALS( Workspace->getNumberHistograms(), 2584 )
    TS_ASSERT_EQUALS( Workspace->blocksize(), 1675 )
    TS_ASSERT_EQUALS( Workspace->size(), 4328200 )
    
    TS_ASSERT_THROWS_NOTHING( Workspace->readX(0) )
  }

  void testCast()
  {
    TS_ASSERT( dynamic_cast<ManagedWorkspace2D*>(Workspace) )
    TS_ASSERT( dynamic_cast<Workspace2D*>(Workspace) )
    TS_ASSERT( dynamic_cast<Mantid::API::Workspace*>(Workspace) )
  }

  void testId()
  {
    TS_ASSERT( ! Workspace->id().compare("ManagedRawFileWorkspace2D") )
  }

  void testData()
  {
    ManagedRawFileWorkspace2D ws("../../../../../Test/AutoTestData/HET15869.raw");

    const MantidVec& x0 = ws.readX(0);
    TS_ASSERT_EQUALS( x0[0], 5. )
    TS_ASSERT_EQUALS( x0[10], 7.5 )
    const MantidVec& x100 = ws.readX(100);
    TS_ASSERT_EQUALS( x100[0], 5. )
    TS_ASSERT_EQUALS( x100[10], 7.5 )

    const MantidVec& y0 = ws.readY(0);
    TS_ASSERT_EQUALS( y0[0], 0. )
    TS_ASSERT_EQUALS( y0[10], 1. )
    const MantidVec& y100 = ws.readY(100);
    TS_ASSERT_EQUALS( y100[0], 1. )
    TS_ASSERT_EQUALS( y100[10], 1. )

  }

  void testChanges()
  {
    ManagedRawFileWorkspace2D ws("../../../../../Test/AutoTestData/HET15869.raw");

    MantidVec& y0 = ws.dataY(0);
    y0[100] = 1234.;

    MantidVec& y1000 = ws.dataY(1000);
    y1000[200] = 4321.;

    TS_ASSERT_EQUALS( ws.dataY(0)[100], 1234. )
    TS_ASSERT_EQUALS( ws.dataY(1000)[200], 4321. )
    TS_ASSERT_EQUALS( ws.readY(0)[100], 1234. )
    TS_ASSERT_EQUALS( ws.readY(1000)[200], 4321. )

  }

  // Test is taken from LoadRawTest
  void testLoadRaw2()
  {
    ConfigServiceImpl& conf = ConfigService::Instance();
    const std::string managed = "ManagedWorkspace.LowerMemoryLimit";
    const std::string oldValue = conf.getString(managed);
    conf.setString(managed,"0");

    LoadRaw2 loader;
    if ( !loader.isInitialized() ) loader.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(),std::runtime_error);

      std::string    inputFile = "HET15869.raw";

    // Now set it...
    loader.setPropertyValue("Filename", inputFile);

    std::string outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_const_sptr output2D = boost::dynamic_pointer_cast<const Workspace2D>(output);
    TS_ASSERT(boost::dynamic_pointer_cast<const ManagedRawFileWorkspace2D>(output2D) )
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 2584);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(99)) == (output2D->dataX(1734)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(673).size(), output2D->dataY(2111).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(999)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(999)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(999)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output2D-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output2D->run().getProtonCharge(), 171.0353, 0.0001 )

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<IInstrument> i = output2D->getInstrument();
    boost::shared_ptr<Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::Detector> ptrDet103 = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
   // boost::shared_ptr<Sample> sample = output2D->getSample();
    Property *l_property = output2D->run().getLogData( std::string("TEMP1") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );

    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    const SpectraDetectorMap& map= output2D->spectraMap();

    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map.nElements(),24964);

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(map.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra 2084
    TS_ASSERT_EQUALS(map.ndet(2084),10);
    // Check the id number of all pixels contributing
    std::vector<int> detectorgroup;
    detectorgroup=map.getDetectors(2084);
    std::vector<int>::const_iterator it;
    int pixnum=101191;
    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
    TS_ASSERT_EQUALS(*it,pixnum++);

    // Test with spectra that does not exist
    // Test that number of pixel=0
    TS_ASSERT_EQUALS(map.ndet(5),0);
    // Test that trying to get the Detector throws.
    std::vector<int> test = map.getDetectors(5);
    TS_ASSERT(test.empty());
    
    AnalysisDataService::Instance().remove(outputSpace);
    conf.setString(managed,oldValue);
  }

private:
  ManagedRawFileWorkspace2D* Workspace;
};

#endif /*ManagedRawFileWorkspace2DTEST_H_*/
