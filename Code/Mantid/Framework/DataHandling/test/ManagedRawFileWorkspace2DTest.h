#ifndef ManagedRawFileWorkspace2DTEST_H_
#define ManagedRawFileWorkspace2DTEST_H_

#include "MantidAPI/FileFinder.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <cxxtest/TestSuite.h>

using Mantid::MantidVec;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class ManagedRawFileWorkspace2DTest : public CxxTest::TestSuite
{
public:

  static ManagedRawFileWorkspace2DTest *createSuite() { return new ManagedRawFileWorkspace2DTest(); }
  static void destroySuite(ManagedRawFileWorkspace2DTest *suite) { delete suite; }

  ManagedRawFileWorkspace2DTest()
  {
    file = FileFinder::Instance().getFullPath("HET15869.raw");
    Workspace = new ManagedRawFileWorkspace2D(file,2);
  }

  virtual ~ManagedRawFileWorkspace2DTest()
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
    ManagedRawFileWorkspace2D ws(file);

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
    ManagedRawFileWorkspace2D ws(file);
    // Need to ensure that the number of writes is greater than the MRUList size
    // so that we check the read/write from the file
    // There is no public API to find the size so this will have to do.
    const size_t nhist = 400;
    for(size_t i = 0; i < nhist; ++i)
    {
      MantidVec& y0 = ws.dataY(i);
      y0[0] = 100.0;
    }
    
    // Check that we have actually changed it
    for(size_t i = 0; i < nhist; ++i)
    {
      const MantidVec& y0 = ws.readY(i);
      const std::string msg = "The first value at index " + boost::lexical_cast<std::string>(i) + " does not have the expected value.";
      TSM_ASSERT_EQUALS(msg, y0[0], 100.0 );
    }
  }

  // Test is taken from LoadRawTest
  void testLoadRaw2()
  {
    // Make sure we go managed
    ConfigServiceImpl& conf = ConfigService::Instance();
    const std::string managed = "ManagedWorkspace.LowerMemoryLimit";
    const std::string oldValue = conf.getString(managed);
    conf.setString(managed,"0");
    const std::string managed2 = "ManagedRawFileWorkspace.DoNotUse";
    const std::string oldValue2 = conf.getString(managed2);
    conf.setString(managed2,"0");

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
    boost::shared_ptr<const Mantid::Geometry::Instrument> i = output2D->getInstrument();
    boost::shared_ptr<const Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<const Mantid::Geometry::Detector> ptrDet103 = boost::dynamic_pointer_cast<const Mantid::Geometry::Detector>(i->getDetector(103));
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
    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS( output2D->getSpectrum(6)->getDetectorIDs().size(), 1);   // rummap.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra 2084 (workspace index 2083)
    TS_ASSERT_EQUALS( output2D->getSpectrum(2083)->getDetectorIDs().size(), 10);   //map.ndet(2084),10);

    // Check the id number of all pixels contributing
    std::set<detid_t> detectorgroup;
    detectorgroup = output2D->getSpectrum(2083)->getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    int pixnum=101191;
    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
      TS_ASSERT_EQUALS(*it,pixnum++);

    //----------------------------------------------------------------------
    // Test new-style spectrum/detector number retrieval
    //----------------------------------------------------------------------
    // Just test a few....
    TS_ASSERT_EQUALS( output2D->getAxis(1)->spectraNo(0), 1 );
    TS_ASSERT_EQUALS( output2D->getSpectrum(0)->getSpectrumNo(), 1 );
    TS_ASSERT( output2D->getSpectrum(0)->hasDetectorID(601) );
    TS_ASSERT_EQUALS( output2D->getDetector(0)->getID(), 601);
    TS_ASSERT_EQUALS( output2D->getAxis(1)->spectraNo(1500), 1501 );
    TS_ASSERT_EQUALS( output2D->getSpectrum(1500)->getSpectrumNo(), 1501 );
    TS_ASSERT( output2D->getSpectrum(1500)->hasDetectorID(405049) );
    TS_ASSERT_EQUALS( output2D->getDetector(1500)->getID(), 405049);
    TS_ASSERT_EQUALS( output2D->getAxis(1)->spectraNo(2580), 2581 );
    TS_ASSERT_EQUALS( output2D->getSpectrum(2580)->getSpectrumNo(), 2581 );
    TS_ASSERT( output2D->getSpectrum(2580)->hasDetectorID(310217) );
    TS_ASSERT_EQUALS( output2D->getDetector(2580)->getID(), 310217);

    AnalysisDataService::Instance().remove(outputSpace);
    conf.setString(managed,oldValue);
    conf.setString(managed2,oldValue2);
  }

private:
  ManagedRawFileWorkspace2D* Workspace;
  std::string file;
};

////------------------------------------------------------------------------------
//// Performance test
////------------------------------------------------------------------------------
//
//class NOTATEST : public CxxTest::TestSuite
//{
//private:
//  const std::string outputSpace;
//
//public:
//  // This pair of boilerplate methods prevent the suite being created statically
//  // This means the constructor isn't called when running other tests
//  static NOTATEST *createSuite() { return new NOTATEST(); }
//  static void destroySuite( NOTATEST *suite ) { delete suite; }
//
//  NOTATEST() : outputSpace("wishWS")
//  {
//    // Load the instrument alone so as to isolate the raw file loading time from the instrument loading time
//    IAlgorithm * loader = FrameworkManager::Instance().createAlgorithm("LoadEmptyInstrument");
//    loader->setPropertyValue("Filename","WISH_Definition.xml");
//    loader->setPropertyValue("OutputWorkspace", "InstrumentOnly");
//    TS_ASSERT( loader->execute() );
//  }
//
//  // This should take ~no time. If it does an unacceptable change has occurred!
//  void testLoadTime()
//  {
//    // Make sure we go managed
//    ConfigServiceImpl& conf = ConfigService::Instance();
//    const std::string managed = "ManagedWorkspace.LowerMemoryLimit";
//    const std::string oldValue = conf.getString(managed);
//    conf.setString(managed,"0");
//    const std::string managed2 = "ManagedRawFileWorkspace.DoNotUse";
//    const std::string oldValue2 = conf.getString(managed2);
//    conf.setString(managed2,"0");
//    const std::string datapath = "datasearch.directories";
//    std::string pathValue = conf.getString(datapath);
//    pathValue.append(";../../Data/SystemTests/");
//    conf.setString(datapath,pathValue);
//
//    IAlgorithm * loader = FrameworkManager::Instance().createAlgorithm("LoadRaw");
//    //IAlgorithm_sptr loader = AlgorithmFactory::Instance().create("LoadRaw");
//    loader->setPropertyValue("Filename","WISH00016748.raw");
//    loader->setPropertyValue("OutputWorkspace",outputSpace);
//    TS_ASSERT( loader->execute() );
//
//    conf.setString(managed,oldValue);
//    conf.setString(managed2,oldValue2);
//  }
//
//  // This also should be very quick (nothing should get written to disk)
//  void testReadValues()
//  {
//    MatrixWorkspace_const_sptr ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(outputSpace);
//    TS_ASSERT( ws );
//
//    double x(0),y(0),e(0);
//    for ( std::size_t i = 0 ; i < ws->getNumberHistograms() ; ++i )
//    {
//      x = ws->readX(i)[0];
//      y = ws->readY(i)[0];
//      e = ws->readE(i)[0];
//    }
//
//    TS_ASSERT( x > 0.0 );
//    TS_ASSERT( y == 0.0 );
//    TS_ASSERT( e == 0.0 );
//
//    AnalysisDataService::Instance().remove(outputSpace);
//  }
//
//};

#endif /*ManagedRawFileWorkspace2DTEST_H_*/
