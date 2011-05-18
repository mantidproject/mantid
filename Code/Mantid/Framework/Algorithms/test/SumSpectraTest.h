#ifndef SUMSPECTRATEST_H_
#define SUMSPECTRATEST_H_

#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class SumSpectraTest : public CxxTest::TestSuite
{
public:
  static SumSpectraTest *createSuite() { return new SumSpectraTest(); }
  static void destroySuite(SumSpectraTest *suite) { delete suite; }

  SumSpectraTest()
  {
    inputSpace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10,102,true);
    inputSpace->instrumentParameters().addBool(inputSpace->getDetector(1).get(),"masked",true);
  }

  ~SumSpectraTest()
  {
    AnalysisDataService::Instance().clear();
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }


  void testExecWithLimits()
  {
    if ( !alg.isInitialized() ) alg.initialize();

    // Set the properties
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace",inputSpace) );
    const std::string outputSpace1 = "SumSpectraOut1";
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace",outputSpace1) );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("StartWorkspaceIndex","1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("EndWorkspaceIndex","3") );

    size_t nspecEntries(0);
    const Mantid::API::SpectraDetectorMap & specMap_in = inputSpace->spectraMap();
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
    size_t max;
    TS_ASSERT_EQUALS( max = inputSpace->blocksize(), output2D->blocksize());
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS( x.size(), 103 );
    TS_ASSERT_EQUALS( y.size(), 102 );
    TS_ASSERT_EQUALS( e.size(), 102 );

    for (size_t i = 0; i < max; ++i)
    {
      TS_ASSERT_EQUALS( x[i], inputSpace->readX(0)[i] );
      TS_ASSERT_EQUALS( y[i], inputSpace->readY(2)[i]+inputSpace->readY(3)[i] );
      TS_ASSERT_DELTA( e[i], std::sqrt(inputSpace->readY(2)[i]+inputSpace->readY(3)[i]), 1.0e-10 );
    }

    // Check the detectors mapped to the single spectra
    const SpectraDetectorMap & specMap_out = output2D->spectraMap();
    const int newSpectrumNo = 1;
    TS_ASSERT_EQUALS( specMap_out.ndet(newSpectrumNo), nspecEntries);

    // And their values
    std::vector<detid_t> dets = specMap_out.getDetectors(newSpectrumNo);
    if( dets.size() == 0 ) 
    {
      TS_FAIL("SpectraMap has been remapped incorrectly");
      return;
    }
    TS_ASSERT_EQUALS(dets[0],2);
    TS_ASSERT_EQUALS(dets[1],3);
  }

  void testExecWithoutLimits()
  {
    Mantid::Algorithms::SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set the properties
    alg2.setProperty("InputWorkspace",inputSpace);
    const std::string outputSpace2 = "SumSpectraOut2";
    alg2.setPropertyValue("OutputWorkspace",outputSpace2);
    alg2.setProperty("IncludeMonitors",false);

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS( alg2.setPropertyValue("StartWorkspaceIndex","-1"), std::invalid_argument) ;

    size_t nspecEntries(0);
    const size_t nHist(inputSpace->getNumberHistograms());
    const SpectraDetectorMap & specMap_in = inputSpace->spectraMap();
    // Spectra at workspace index 1 is masked, 8 & 9 are monitors
    for( size_t i = 1; i < nHist-2; ++i )
    {
      nspecEntries += specMap_in.ndet(static_cast<specid_t>(i));
    }

    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace2));
    Workspace2D_const_sptr output2D = boost::dynamic_pointer_cast<const Workspace2D>(output);

    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS( x.size(), 103 );
    TS_ASSERT_EQUALS( y.size(), 102 );
    TS_ASSERT_EQUALS( e.size(), 102 );

    // Check a few bins
    TS_ASSERT_EQUALS( x[0], inputSpace->readX(0)[0] );
    TS_ASSERT_EQUALS( x[50], inputSpace->readX(0)[50] );
    TS_ASSERT_EQUALS( x[100], inputSpace->readX(0)[100] );
    TS_ASSERT_EQUALS( y[7], 14 );
    TS_ASSERT_EQUALS( y[38], 14 );
    TS_ASSERT_EQUALS( y[72], 14 );
    TS_ASSERT_DELTA( e[28], std::sqrt(y[28]), 0.00001 );
    TS_ASSERT_DELTA( e[47], std::sqrt(y[47]), 0.00001 );
    TS_ASSERT_DELTA( e[99], std::sqrt(y[99]), 0.00001 );

    // Check the detectors mapped to the single spectra
    const SpectraDetectorMap & specMap_out = output2D->spectraMap();
    const int newSpectrumNo(0);
    TS_ASSERT_EQUALS( specMap_out.ndet(newSpectrumNo), nspecEntries);

    // And their values
    std::vector<detid_t> dets = specMap_out.getDetectors(newSpectrumNo);
    if( dets.size() == 0 ) 
    {
      TS_FAIL("SpectraMap has been remapped incorrectly");
      return;
    }
    TS_ASSERT_EQUALS(dets[0], 0);
    TS_ASSERT_EQUALS(dets[1], 2);
    TS_ASSERT_EQUALS(dets[2], 3);
    TS_ASSERT_EQUALS(dets[3], 4);
    TS_ASSERT_EQUALS(dets[4], 5);
    TS_ASSERT_EQUALS(dets[5], 6);
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
    AnalysisDataService::Instance().addOrReplace(inName, input);

    Mantid::Algorithms::SumSpectra alg2;
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
  }

private:
  Mantid::Algorithms::SumSpectra alg;   // Test with range limits
  MatrixWorkspace_sptr inputSpace;
};

#endif /*SUMSPECTRATEST_H_*/
