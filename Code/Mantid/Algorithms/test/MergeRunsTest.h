#ifndef MERGERUNSTEST_H_
#define MERGERUNSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/MergeRuns.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class MergeRunsTest : public CxxTest::TestSuite
{
public:
  MergeRunsTest()
  {
    AnalysisDataService::Instance().add("in1",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,1));
    AnalysisDataService::Instance().add("in2",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,1));
    AnalysisDataService::Instance().add("in3",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,1));
    AnalysisDataService::Instance().add("in4",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,5,20));
    AnalysisDataService::Instance().add("in5",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,5,3.5,2));
    AnalysisDataService::Instance().add("in6",WorkspaceCreationHelper::Create2DWorkspaceBinned(3,3,2,2));
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
  void xtestExecAllEvents()
  {
    //Event workspaces with 100 events
    AnalysisDataService::Instance().add("ev1",WorkspaceCreationHelper::CreateEventWorkspace(3,10,100));
    AnalysisDataService::Instance().add("ev2",WorkspaceCreationHelper::CreateEventWorkspace(3,10,100));
    AnalysisDataService::Instance().add("ev3",WorkspaceCreationHelper::CreateEventWorkspace(3,10,100));

    if ( !merge.isInitialized() ) merge.initialize();

    TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("InputWorkspaces","ev1,ev2,ev3") );
    TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("OutputWorkspace","outWS") );

    TS_ASSERT_THROWS_NOTHING( merge.execute() );
    TS_ASSERT( merge.isExecuted() );

    //Get the output event workspace
    EventWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("outWS")) );
    //This checks that it is indeed an EW
    TS_ASSERT( output  );

    //Should have 300 total events
    TS_ASSERT_EQUALS( output->getNumberEvents(), 300);

    AnalysisDataService::Instance().remove("outWS");
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
