#ifndef SUMSPECTRATEST_H_
#define SUMSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "WorkspaceCreationHelper.hh"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SumSpectraTest : public CxxTest::TestSuite
{
public:

  SumSpectraTest()
  {
    outputSpace1 = "SumSpectraOut1";
    outputSpace2 = "SumSpectraOut2";
    inputSpace = "SumSpectraIn";

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace",inputSpace);
    loader.execute();

    // Mask an input spectrum
    Mantid::DataHandling::MaskDetectors mask;
    mask.initialize();
    mask.setPropertyValue("Workspace",inputSpace);
    mask.setPropertyValue("WorkspaceIndexList","1");
    mask.execute();
  }

  ~SumSpectraTest()
  {}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace",inputSpace) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace",outputSpace1) );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("StartWorkspaceIndex","1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("EndWorkspaceIndex","3") );
  }


  void testExecWithLimits()
  {
    if ( !alg.isInitialized() ) alg.initialize();

    // Get back the input workspace
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));
    Workspace2D_const_sptr input2D = boost::dynamic_pointer_cast<const Workspace2D>(input);


    int nspecEntries(0);
    const int nHist(input2D->getNumberHistograms());
    const SpectraDetectorMap & specMap_in = input2D->spectraMap();
    // Spectra at workspace index 1 is masked
    for( int i = 2; i < 4; ++i )
    {
      nspecEntries += specMap_in.ndet(i);
    }

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace1));

    Workspace2D_const_sptr output2D = boost::dynamic_pointer_cast<const Workspace2D>(output);
    int max;
    TS_ASSERT_EQUALS( max = input2D->blocksize(), output2D->blocksize());
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS( x.size(), 103 );
    TS_ASSERT_EQUALS( y.size(), 102 );
    TS_ASSERT_EQUALS( e.size(), 102 );

    for (int i = 0; i < max; ++i)
    {
      TS_ASSERT_EQUALS( x[i], input2D->readX(0)[i] );
      TS_ASSERT_EQUALS( y[i], input2D->readY(2)[i]+input2D->readY(3)[i] );
      TS_ASSERT_DELTA( e[i], std::sqrt(input2D->readY(2)[i]+input2D->readY(3)[i]), 1.0e-10 );
    }



    // Check the detectors mapped to the single spectra
    const SpectraDetectorMap & specMap_out = output2D->spectraMap();
    const int newSpectrumNo = 2;
    TS_ASSERT_EQUALS( specMap_out.ndet(newSpectrumNo), nspecEntries);

    // And their values
    std::vector<int> dets = specMap_out.getDetectors(newSpectrumNo);
    if( dets.size() == 0 ) 
    {
      TS_FAIL("SpectraMap has been remapped incorrectly");
      return;
    }
    TS_ASSERT_EQUALS(dets[0],3);
    TS_ASSERT_EQUALS(dets[1],4);

    AnalysisDataService::Instance().remove(outputSpace1);
  }

  void testExecWithoutLimits()
  {
    SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set the properties
    alg2.setPropertyValue("InputWorkspace",inputSpace);
    alg2.setPropertyValue("OutputWorkspace",outputSpace2);
    alg2.setProperty("IncludeMonitors",false);

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS( alg2.setPropertyValue("StartWorkspaceIndex","-1"), std::invalid_argument) ;

    // Get back the input workspace
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));
    Workspace2D_const_sptr input2D = boost::dynamic_pointer_cast<const Workspace2D>(input);

    int nspecEntries(0);
    const int nHist(input2D->getNumberHistograms());
    const SpectraDetectorMap & specMap_in = input2D->spectraMap();
    // Spectra at workspace index 1 is masked and 0 is a monitor
    for( int i = 2; i < nHist; ++i )
    {
      nspecEntries += specMap_in.ndet(i);
    }

    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace2));
    Workspace2D_const_sptr output2D = boost::dynamic_pointer_cast<const Workspace2D>(output);

    int max = output2D->blocksize();
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1);
    double yy[5] = {50,55,60,65,70};

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS( x.size(), 103 );
    TS_ASSERT_EQUALS( y.size(), 102 );
    TS_ASSERT_EQUALS( e.size(), 102 );

    // Check a few bins
    TS_ASSERT_EQUALS( x[0], input2D->readX(0)[0] );
    TS_ASSERT_EQUALS( x[50], input2D->readX(0)[50] );
    TS_ASSERT_EQUALS( x[100], input2D->readX(0)[100] );
    TS_ASSERT_EQUALS( y[7], 9 );
    TS_ASSERT_EQUALS( y[38], 16277 );
    TS_ASSERT_EQUALS( y[72], 7093 );
    TS_ASSERT_EQUALS( e[28], std::sqrt(y[28]) );
    TS_ASSERT_EQUALS( e[47], std::sqrt(y[47]) );
    TS_ASSERT_EQUALS( e[99], std::sqrt(y[99]) );

    // Check the detectors mapped to the single spectra
    const SpectraDetectorMap & specMap_out = output2D->spectraMap();
    const int newSpectrumNo(1);
    TS_ASSERT_EQUALS( specMap_out.ndet(newSpectrumNo), nspecEntries);

    // And their values
    std::vector<int> dets = specMap_out.getDetectors(newSpectrumNo);
    if( dets.size() == 0 ) 
    {
      TS_FAIL("SpectraMap has been remapped incorrectly");
      return;
    }
    TS_ASSERT_EQUALS(dets[0], 3);
    TS_ASSERT_EQUALS(dets[1], 4);
    TS_ASSERT_EQUALS(dets[2], 5);
    TS_ASSERT_EQUALS(dets[3], 6);
    TS_ASSERT_EQUALS(dets[4], 7);
    TS_ASSERT_EQUALS(dets[5], 8);

    AnalysisDataService::Instance().remove(inputSpace);
    AnalysisDataService::Instance().remove(outputSpace1);
  }

  void testExecEvent_inplace()
  {
    dotestExecEvent("testEvent", "testEvent", "5,10-15");
  }

  void testExecEvent_copy()
  {
    dotestExecEvent("testEvent", "testEvent2", "5,10-15");
  }

  void testExecEvent_going_too_far()
  {
    dotestExecEvent("testEvent", "testEvent2", "5,10-15, 500-600");
  }

  void dotestExecEvent(std::string inName, std::string outName, std::string indices_list)
  {
    int numPixels = 100;
    int numBins = 20;
    int numEvents = 20;
    EventWorkspace_sptr input = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numEvents);
    AnalysisDataService::Instance().add(inName, input);

    SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set the properties
    alg2.setPropertyValue("InputWorkspace",inName);
    alg2.setPropertyValue("OutputWorkspace",outName);
    alg2.setProperty("IncludeMonitors",false);
    alg2.setPropertyValue("ListOfWorkspaceIndices", indices_list);
    alg2.setPropertyValue("StartWorkspaceIndex","4") ;
    alg2.setPropertyValue("EndWorkspaceIndex","6") ;
    //This list has 9 entries: 4,5,6, 10,11,12,13,14,15

    alg2.execute();
    TS_ASSERT(alg2.isExecuted());

    EventWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outName));
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(output->getNumberEvents(), 9 * numEvents);
    TS_ASSERT_EQUALS(input->readX(0).size(), output->readX(0).size());

    AnalysisDataService::Instance().remove(inName);
    if (inName != outName)
      AnalysisDataService::Instance().remove(outName);
  }

private:
  SumSpectra alg;   // Test with range limits
  std::string outputSpace1;
  std::string outputSpace2;
  std::string inputSpace;
};

#endif /*SUMSPECTRATEST_H_*/
