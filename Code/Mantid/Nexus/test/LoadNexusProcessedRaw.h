#ifndef LOADNEXUSPROCESSEDTESTRAW_H_
#define LOADNEXUSPROCESSEDTESTRAW_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"
//
#include "MantidDataHandling/LoadRaw.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidNexus/SaveNexusProcessed.h"
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidNexus/LoadNeXus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Component.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class LoadNexusProcessedTestRaw : public CxxTest::TestSuite
{
public:

  LoadNexusProcessedTestRaw()
  {
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }

void testExecOnLoadraw()
{
    // use SaveNexusProcessed to build a test file to load
    // for this use LoadRaw
    std::string inputFile = "../../../../Test/Data/HET15869.RAW";
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "het15869";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    map=output2D->getSpectraMap();
    //
    if ( !saveNexusP.isInitialized() ) saveNexusP.initialize();

    saveNexusP.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "testSaveLoadrawHET.nxs";
    remove(outputFile.c_str());
    //std::string entryName = "junk"; // not used
    std::string dataName = "spectra";
    std::string title = "Workspace from Loadraw HET15869";
    saveNexusP.setPropertyValue("FileName", outputFile);
    //saveNexusP.setPropertyValue("EntryName", entryName);
    saveNexusP.setPropertyValue("Title", title);

    TS_ASSERT_THROWS_NOTHING(saveNexusP.execute());
    TS_ASSERT( saveNexusP.isExecuted() );

}

  void testExecRaw()
  {
    // test LoadNexusProcessed reading the data from SNP on Loadraw HET15869

    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();

    // specify name of workspace
    myOutputSpace="testLNP3";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("OutputWorkspace", myOutputSpace));
    // file name to load
    inputFile = "testSaveLoadrawHET.nxs";
    entryNumber=1;
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("FileName", inputFile));
    algToBeTested.setProperty("EntryNumber", entryNumber);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("FileName") )
    TS_ASSERT( ! result.compare(inputFile));
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( ! result.compare(myOutputSpace));
    int res=-1;
    TS_ASSERT_THROWS_NOTHING( res = algToBeTested.getProperty("EntryNumber") )
    TS_ASSERT( res==entryNumber);

    //
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(myOutputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
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
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output->getSample()->getProtonCharge(), 171.0353, 0.0001 )

    //
    // check that the instrument data has been loaded, copied from LoadInstrumentTest
    //
    boost::shared_ptr<Instrument> i = output->getInstrument();
    std::cerr << "Count = " << i.use_count();
    Component* source = i->getSource();
    TS_ASSERT( source != NULL);
    if(source != NULL )
    {
        TS_ASSERT_EQUALS( source->getName(), "undulator");
        TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);
    
        Component* samplepos = i->getSample();
        TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
        TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);
    
        Detector *ptrDet103 = dynamic_cast<Detector*>(i->getDetector(103));
        TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
        TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
        TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
        TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
        double d = ptrDet103->getPos().distance(samplepos->getPos());
        TS_ASSERT_DELTA(d,2.512,0.0001);
        double cmpDistance = ptrDet103->getDistance(*samplepos);
        TS_ASSERT_DELTA(cmpDistance,2.512,0.0001);
    }
    //
    // Get the map from the workspace : TESTS from LoadMappingTest.h
    map=output->getSpectraMap();
    TS_ASSERT( map != NULL);
    if(map != NULL )
    {

        // Check the total number of elements in the map for HET
        //TS_ASSERT_EQUALS(map->nElements(),24964);
        // above is value from LoadRaw, but only 12124 seem used and are
        // recorded in Nxus file
        TS_ASSERT_EQUALS(map->nElements(),12124);
    
        // Test one to one mapping, for example spectra 6 has only 1 pixel
        TS_ASSERT_EQUALS(map->ndet(6),1);
    
        // Test one to many mapping, for example 10 pixels contribute to spectra 2084
        TS_ASSERT_EQUALS(map->ndet(2084),10);
    
        // Check the id number of all pixels contributing
        std::vector<Mantid::Geometry::IDetector*> detectorgroup;
        detectorgroup=map->getDetectors(2084);
        std::vector<Mantid::Geometry::IDetector*>::iterator it;
        int pixnum=101191;
        for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
        TS_ASSERT_EQUALS((*it)->getID(),pixnum++);
     
        // Test with spectra that does not exist
        // Test that number of pixel=0
        TS_ASSERT_EQUALS(map->ndet(5),0);
        // Test that trying to get the Detector throws.
        boost::shared_ptr<Mantid::Geometry::IDetector> test;
        TS_ASSERT_THROWS(test=map->getDetector(5),std::runtime_error);
        //
    }

    remove(outputFile.c_str());

  }


private:
  LoadNexusProcessed algToBeTested;
  std::string inputFile;
  int entryNumber;
  Workspace2D myworkspace;

  std::string myOutputSpace;

  SaveNexusProcessed saveNexusP;
  Mantid::DataHandling::LoadRaw loader;
  std::string outputSpace;
  std::string outputFile;
  boost::shared_ptr<SpectraDetectorMap> map;
};

#endif /*LOADNEXUSPROCESSEDTESTRAW_H_*/
