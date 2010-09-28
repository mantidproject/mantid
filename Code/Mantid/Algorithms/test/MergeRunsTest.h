#ifndef MERGERUNSTEST_H_
#define MERGERUNSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"
#include <stdarg.h>

#include "MantidAlgorithms/MergeRuns.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class MergeRunsTest : public CxxTest::TestSuite
{
public:

  EventWorkspace_sptr ev1, ev2, ev3, ev4, ev5,ev6,evg1, evg2, evg3;

  MergeRunsTest()
  {
    AnalysisDataService::Instance().add("in1",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,1));
    AnalysisDataService::Instance().add("in2",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,1));
    AnalysisDataService::Instance().add("in3",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,1));
    AnalysisDataService::Instance().add("in4",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,5,20));
    AnalysisDataService::Instance().add("in5",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,5,3.5,2));
    AnalysisDataService::Instance().add("in6",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,3,2,2));
  }






  //-----------------------------------------------------------------------------------------------
  void testExec_Event_CNCS()
  {
    std::string eventfile1( "../../../../Test/AutoTestData/CNCS_12772_neutron_event.dat" );
    std::string eventfile2( "../../../../Test/AutoTestData/CNCS_7850_neutron_event.dat" );
    DataHandling::LoadEventPreNeXus * eventLoader;

    TimeSeriesProperty<double>* log;
    EventWorkspace_sptr output;
    int log1, log2, logTot;
    int nev1, nev2, nevTot;
    double pc1, pc2, pcTot;

    eventLoader = new DataHandling::LoadEventPreNeXus(); eventLoader->initialize();
    eventLoader->setPropertyValue("EventFilename", eventfile1);
    eventLoader->setPropertyValue("MappingFilename", "../../../../Test/AutoTestData/CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "cncs1");
    eventLoader->setProperty("PadEmptyPixels", true);
    TS_ASSERT( eventLoader->execute() );
    delete eventLoader;

    output =  boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("cncs1"));
    log = dynamic_cast< TimeSeriesProperty<double>* > ( output->mutableRun().getProperty("ProtonCharge") );
    log1 = log->realSize();
    nev1 = output->getNumberEvents();
    pc1 = output->mutableRun().getProtonCharge();

    //For the second one, we wont pad the pixels
    eventLoader = new DataHandling::LoadEventPreNeXus(); eventLoader->initialize();
    eventLoader->setPropertyValue("EventFilename", eventfile2);
    eventLoader->setPropertyValue("MappingFilename", "../../../../Test/AutoTestData/CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "cncs2");
    eventLoader->setProperty("PadEmptyPixels", false);
    TS_ASSERT( eventLoader->execute() );
    delete eventLoader;

    output =  boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("cncs2"));
    log = dynamic_cast< TimeSeriesProperty<double>* > ( output->mutableRun().getProperty("ProtonCharge") );
    log2 = log->realSize();
    nev2 = output->getNumberEvents();
    pc2 = output->mutableRun().getProtonCharge();

    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","cncs1,cncs2");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );
    output =  boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    TS_ASSERT( output );

    //This many pixels total at CNCS
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 51200);

    log = dynamic_cast< TimeSeriesProperty<double>* > ( output->mutableRun().getProperty("ProtonCharge") );
    logTot = log->realSize();
    nevTot = output->getNumberEvents();
    pcTot = output->mutableRun().getProtonCharge();

    //Total # of log entries
    TS_ASSERT_EQUALS( logTot, log1+log2 );
    //Summed up the proton charge
    TS_ASSERT_EQUALS( pcTot, pc1+pc2 );
    //Total events counted.
    TS_ASSERT_EQUALS( nevTot, nev1+nev2 );

  }








  std::vector<int> makeVector(int num, ...)
  {
    std::vector<int> retVal;
    va_list vl;
    va_start( vl, num );
    for (int i=0; i<num; i++)
      retVal.push_back( va_arg( vl, int) );
    return retVal;
  }



  void EventSetup()
  {
    ev1 = WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 3);
    AnalysisDataService::Instance().addOrReplace("ev1", boost::dynamic_pointer_cast<MatrixWorkspace>(ev1)); // 100 ev
    AnalysisDataService::Instance().addOrReplace("ev2", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2))); //200 ev
    AnalysisDataService::Instance().addOrReplace("ev3", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2, 100))); //200 events per spectrum, but the spectra are at different pixel ids
    //Make one with weird units
    MatrixWorkspace_sptr ev4 = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2, 100));
    ev4->setYUnit("Microfurlongs per Megafortnights");
    AnalysisDataService::Instance().addOrReplace("ev4_weird_units",ev4);
    AnalysisDataService::Instance().addOrReplace("ev5", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(5,10,100, 0.0, 1.0, 2, 100))); //200 events per spectrum, but the spectra are at different pixel ids
    ev6 = WorkspaceCreationHelper::CreateEventWorkspace(6,10,100, 0.0, 1.0, 3);//ids 0-5
    AnalysisDataService::Instance().addOrReplace("ev6", boost::dynamic_pointer_cast<MatrixWorkspace>(ev6));
    //a 2d workspace with the value 2 in each bin
    AnalysisDataService::Instance().addOrReplace("in2D", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 0.0, 1.0));


    std::vector< std::vector<int> > groups;

    groups.clear();
    groups.push_back( makeVector(3,  0,1,2) );
    groups.push_back( makeVector(3,  3,4,5) );
    evg1 = WorkspaceCreationHelper::CreateGroupedEventWorkspace(groups, 100);
    AnalysisDataService::Instance().addOrReplace("evg1", boost::dynamic_pointer_cast<MatrixWorkspace>(evg1));

    //let's check on the setup
    TS_ASSERT_EQUALS( evg1->getNumberEvents(), 600);
    TS_ASSERT_EQUALS( evg1->getNumberHistograms(), 2);
    TS_ASSERT( evg1->getEventList(0).hasDetectorID(0));
    TS_ASSERT( evg1->getEventList(0).hasDetectorID(1));
    TS_ASSERT( evg1->getEventList(0).hasDetectorID(2));
    TS_ASSERT( evg1->getEventList(1).hasDetectorID(3));

    groups.clear();
    groups.push_back( makeVector(2,  3,4) );
    groups.push_back( makeVector(3,  0,1,2) );
    groups.push_back( makeVector(1,  15) );
    evg2 = WorkspaceCreationHelper::CreateGroupedEventWorkspace(groups, 100);
    AnalysisDataService::Instance().addOrReplace("evg2", boost::dynamic_pointer_cast<MatrixWorkspace>(evg2));


  }


  void EventTeardown()
  {
    AnalysisDataService::Instance().remove("ev1");
    AnalysisDataService::Instance().remove("ev2");
    AnalysisDataService::Instance().remove("ev3");
    AnalysisDataService::Instance().remove("ev4_weird_units");
    AnalysisDataService::Instance().remove("ev5");
    AnalysisDataService::Instance().remove("ev6");
    AnalysisDataService::Instance().remove("in2D");
    AnalysisDataService::Instance().remove("evg1");
    AnalysisDataService::Instance().remove("evOUT");
    AnalysisDataService::Instance().remove("out2D");
  }



	void testTheBasics()
	{
    TS_ASSERT_EQUALS( merge.name(), "MergeRuns" );
    TS_ASSERT_EQUALS( merge.version(), 1 );
    TS_ASSERT_EQUALS( merge.category(), "General" );
	}

	void testInit()
	{
	  TS_ASSERT_THROWS_NOTHING( merge.initialize() );
	  TS_ASSERT( merge.isInitialized() );
	}

  //-----------------------------------------------------------------------------------------------
  void testExec()
  {
    if ( !merge.isInitialized() ) merge.initialize();

    TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("InputWorkspaces","in1,in2,in3") );
    TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("OutputWorkspace","outWS") );

    TS_ASSERT_THROWS_NOTHING( merge.execute() );
    TS_ASSERT( merge.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS")) );

    MatrixWorkspace::const_iterator inIt(*(boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("in1"))));
    for (MatrixWorkspace::const_iterator it(*output); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() );
      TS_ASSERT_EQUALS( it->Y(), 6.0 );
      TS_ASSERT_DELTA( it->E(), sqrt(6.0), 0.00001 );
    }

    AnalysisDataService::Instance().remove("outWS");
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_MixingEventAnd2D_gives_a2D()
  {
    EventSetup();
    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","ev1,ev2,in1");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );
    //Not an EventWorkspace
    EventWorkspace_sptr outEvent =  boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    TS_ASSERT( !outEvent );
    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MixedIDs()
  {
    EventSetup();
    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","ev1,ev2,ev3");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Should have 300+600+600 = 1500 total events
    TS_ASSERT_EQUALS( output->getNumberEvents(), 1500);
    //6 unique pixel ids
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 6);

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MismatchedUnits_fail()
  {
    EventSetup();
    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","ev1,ev4_weird_units,ev3");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( !mrg.isExecuted() );
    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MatchingPixelIDs()
  {
    EventSetup();
    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","ev1,ev2");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Should have 300+600
    TS_ASSERT_EQUALS( output->getNumberEvents(), 900);
    //3 unique pixel ids
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 3);

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped1()
  {
    EventSetup();

    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","evg1,ev1");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Total # of events
    TS_ASSERT_EQUALS( output->getNumberEvents(), ev1->getNumberEvents() + evg1->getNumberEvents());
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 2); //2 groups; 0-2 and 3-5

    TS_ASSERT_EQUALS( output->getEventList(0).getNumberEvents(), 600); //300 + 3x100
    TS_ASSERT( output->getEventList(0).hasDetectorID(0));
    TS_ASSERT( output->getEventList(0).hasDetectorID(1));
    TS_ASSERT( output->getEventList(0).hasDetectorID(2));

    TS_ASSERT_EQUALS( output->getEventList(1).getNumberEvents(), 300); //300
    TS_ASSERT( output->getEventList(1).hasDetectorID(3));
    TS_ASSERT( output->getEventList(1).hasDetectorID(4));
    TS_ASSERT( output->getEventList(1).hasDetectorID(5));

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped1_flipped()
  {
    EventSetup();

    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","ev1,evg1");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Total # of events
    TS_ASSERT_EQUALS( output->getNumberEvents(), ev1->getNumberEvents() + evg1->getNumberEvents());
    // Grouped pixel IDs: 0; 1; 2; 012; 345
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 5);
    TS_ASSERT( output->getEventList(0).hasDetectorID(0));
    TS_ASSERT( output->getEventList(1).hasDetectorID(1));
    TS_ASSERT( output->getEventList(2).hasDetectorID(2));
    TS_ASSERT( output->getEventList(3).hasDetectorID(0));
    TS_ASSERT( output->getEventList(3).hasDetectorID(1));
    TS_ASSERT( output->getEventList(3).hasDetectorID(2));
    TS_ASSERT( output->getEventList(4).hasDetectorID(3));
    TS_ASSERT( output->getEventList(4).hasDetectorID(4));
    TS_ASSERT( output->getEventList(4).hasDetectorID(5));

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped2()
  {
    EventSetup();

    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","evg2,ev6");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Total # of events
    TS_ASSERT_EQUALS( output->getNumberEvents(), ev6->getNumberEvents() + evg2->getNumberEvents());
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS( output->getEventList(0).getNumberEvents(), 400); //4 lists were added
    TS_ASSERT_EQUALS( output->getEventList(1).getNumberEvents(), 600);
    TS_ASSERT_EQUALS( output->getEventList(2).getNumberEvents(), 100);
    TS_ASSERT_EQUALS( output->getEventList(3).getNumberEvents(), 100);
    // Groups are 3,4;   0,1,2;   15(from ev6); 5(unused in ev6)
    TS_ASSERT( output->getEventList(0).hasDetectorID(3));
    TS_ASSERT( output->getEventList(0).hasDetectorID(4));
    TS_ASSERT( output->getEventList(1).hasDetectorID(0));
    TS_ASSERT( output->getEventList(1).hasDetectorID(1));
    TS_ASSERT( output->getEventList(1).hasDetectorID(2));
    TS_ASSERT( output->getEventList(2).hasDetectorID(15));
    TS_ASSERT( output->getEventList(3).hasDetectorID(5)); //Leftover from the ev1 workspace

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped3()
  {
    EventSetup();

    MergeRuns mrg;  mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces","evg1,ev1,evg2");
    mrg.setPropertyValue("OutputWorkspace","outWS");
    mrg.execute();
    TS_ASSERT( mrg.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Total # of events
    TS_ASSERT_EQUALS( output->getNumberEvents(), ev1->getNumberEvents() + evg1->getNumberEvents() + evg2->getNumberEvents());
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 3);

    TS_ASSERT_EQUALS( output->getEventList(0).getNumberEvents(), 900); //300 (evg1) + 3x100 (ev1) + 3x100 (evg2 had 012)
    TS_ASSERT( output->getEventList(0).hasDetectorID(0));
    TS_ASSERT( output->getEventList(0).hasDetectorID(1));
    TS_ASSERT( output->getEventList(0).hasDetectorID(2));

    TS_ASSERT_EQUALS( output->getEventList(1).getNumberEvents(), 500); //300 + 2x100 (evg2 had 3,4 only)
    TS_ASSERT( output->getEventList(1).hasDetectorID(3));
    TS_ASSERT( output->getEventList(1).hasDetectorID(4));
    TS_ASSERT( output->getEventList(1).hasDetectorID(5));

    //Leftover 15 from evg2
    TS_ASSERT_EQUALS( output->getEventList(2).getNumberEvents(), 100); // (evg2)
    TS_ASSERT( output->getEventList(2).hasDetectorID(15));

    EventTeardown();
  }


  //-----------------------------------------------------------------------------------------------
	void testInvalidInputs()
	{
	  MergeRuns merge2;
	  TS_ASSERT_THROWS_NOTHING( merge2.initialize() );
	  TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("OutputWorkspace","null") );
	  TS_ASSERT_THROWS( merge2.execute(), std::runtime_error );
    TS_ASSERT( ! merge2.isExecuted() );
    MatrixWorkspace_sptr badIn = WorkspaceCreationHelper::Create2DWorkspace123(10,3,1);
	  badIn->dataX(0) = std::vector<double>(11,2.0);
    AnalysisDataService::Instance().add("badIn",badIn);
    TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("InputWorkspaces","ws1,badIn") );
    TS_ASSERT_THROWS( merge2.execute(), std::runtime_error );
    TS_ASSERT( ! merge2.isExecuted() );
	}

  //-----------------------------------------------------------------------------------------------
	void testNonOverlapping()
	{
	  MergeRuns alg;
	  alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspaces","in1,in4") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","outer") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outer")) );

    const Mantid::MantidVec &X = output->readX(0);
    TS_ASSERT_EQUALS( X.size(), 17 );
    int i;
    for (i = 0; i < 11; ++i)
    {
      TS_ASSERT_EQUALS( X[i], i+1 );
    }
    for (; i < 17; ++i)
    {
      TS_ASSERT_EQUALS( X[i], i+9 );
    }

    AnalysisDataService::Instance().remove("outer");
	}

  //-----------------------------------------------------------------------------------------------
	void testIntersection()
	{
    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspaces","in1,in5") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","outer") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outer")) );

    const Mantid::MantidVec &X = output->readX(0);
    TS_ASSERT_EQUALS( X.size(), 8 );
    int i;
    for (i = 0; i < 3; ++i)
    {
      TS_ASSERT_EQUALS( X[i], i+1 );
    }
    for (; i < 8; ++i)
    {
      TS_ASSERT_EQUALS( X[i], 2*i-0.5 );
    }

    AnalysisDataService::Instance().remove("outer");
	}

  //-----------------------------------------------------------------------------------------------
  void testInclusion()
  {
    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspaces","in6,in1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","outer") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outer")) );

    const Mantid::MantidVec &X = output->readX(0);
    TS_ASSERT_EQUALS( X.size(), 8 );
    int i;
    for (i = 0; i < 2; ++i)
    {
      TS_ASSERT_EQUALS( X[i], i+1 );
    }
    for (; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( X[i], 2*i );
    }
    for (; i < 8; ++i)
    {
      TS_ASSERT_EQUALS( X[i], i+4 );
    }

    AnalysisDataService::Instance().remove("outer");
  }

private:
  MergeRuns merge;
};

#endif /*MERGERUNSTEST_H_*/
