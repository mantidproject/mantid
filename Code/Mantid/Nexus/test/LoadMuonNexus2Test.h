#ifndef LOADMUONNEXUS2TEST_H_
#define LOADMUONNEXUS2TEST_H_

//This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
//

#include <fstream>
//#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonNexus2.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexus2Test// : public CxxTest::TestSuite
{
public:
  
  void testExec()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "../../../Test/Nexus/Muon-v2/argus0026287.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 192 for file inputFile = "argus0026287.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 192);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000-1);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(11)[686], 9);
    TS_ASSERT_EQUALS( output2D->dataY(12)[686], 7);
    TS_ASSERT_EQUALS( output2D->dataY(13)[686], 7);

    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(11)[686], 3);
    TS_ASSERT_DELTA( output2D->dataE(12)[686], 2.646,0.001);
    TS_ASSERT_DELTA( output2D->dataE(13)[686], 2.646,0.001);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.992,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output-> isDistribution() )

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    IInstrument_sptr i = output->getInstrument();
    Mantid::Geometry::IObjComponent_sptr source = i->getSource();
    /*
    Mantid::Geometry::IObjComponent_sptr samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 10.0,0.01);


    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    Mantid::Geometry::Detector *ptrDet103 = dynamic_cast<Mantid::Geometry::Detector*>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    */
    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    //boost::shared_ptr<Sample> sample = output->getSample();
    //Property *l_property = output->sample().getLogData( std::string("beamlog_current") );
    //TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    //std::string timeSeriesString = l_timeSeriesDouble->value();
    //TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "2006-Nov-21 07:03:08  182.8" );
    ////check that sample name has been set correctly
    //TS_ASSERT_EQUALS(output->sample().getName(), "Cr2.7Co0.3Si")
    
	/*
    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    map= output->getSpectraMap();
    
    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map->nElements(),24964);
    
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
    */

    AnalysisDataService::Instance().remove(outputSpace);
   
  }

  void testMinMax()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "../../../Test/Nexus/Muon-v2/argus0026287.nxs");
    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumMin","10");
    nxLoad.setPropertyValue("SpectrumMax","20");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 11);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000-1);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(7)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(10).size() );
    // Check one particular value
    //TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    //TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    //TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testList()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "../../../Test/Nexus/Muon-v2/argus0026287.nxs");
    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumList","1,10,20");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000-1);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(0)) == (output2D->dataX(2)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(0).size(), output2D->dataY(1).size() );
    // Check one particular value
    //TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    //TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    //TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    AnalysisDataService::Instance().remove(outputSpace);
  }
  void testMinMax_List()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "../../../Test/Nexus/Muon-v2/argus0026287.nxs");
    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumMin","10");
    nxLoad.setPropertyValue("SpectrumMax","20");
    nxLoad.setPropertyValue("SpectrumList","30,40,50");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 14);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000-1);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(7)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(10).size() );
    // Check one particular value
    //TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    //TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    //TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    AnalysisDataService::Instance().remove(outputSpace);
  }
};
#endif /*_WIN64*/  
#endif /*LOADMUONNEXUS2TEST_H_*/
