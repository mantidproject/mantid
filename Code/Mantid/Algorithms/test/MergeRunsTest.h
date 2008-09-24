#ifndef MERGERUNSTEST_H_
#define MERGERUNSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/MergeRuns.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class MergeRunsTest : public CxxTest::TestSuite
{
public:
  MergeRunsTest()
  {
    AnalysisDataService::Instance().add("in1",WorkspaceCreationHelper::Create2DWorkspace123(10,3,1));
    AnalysisDataService::Instance().add("in2",WorkspaceCreationHelper::Create2DWorkspace123(10,3,1));
    AnalysisDataService::Instance().add("in3",WorkspaceCreationHelper::Create2DWorkspace123(10,3,1));
  }

	void testName()
	{
    TS_ASSERT_EQUALS( merge.name(), "MergeRuns" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( merge.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( merge.category(), "General" )
	}

	void testInit()
	{
	  TS_ASSERT_THROWS_NOTHING( merge.initialize() )
	  TS_ASSERT( merge.isInitialized() )
	}

	void testExec()
	{
	  if ( !merge.isInitialized() ) merge.initialize();

	  TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("InputWorkspaces","in1,in2,in3") )
	  TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("OutputWorkspace","outWS") )

    TS_ASSERT_THROWS_NOTHING( merge.execute() )
    TS_ASSERT( merge.isExecuted() )

    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("outWS") )

    Workspace::const_iterator inIt(*(AnalysisDataService::Instance().retrieve("in1")));
    for (Workspace::const_iterator it(*output); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() )
      TS_ASSERT_EQUALS( it->Y(), 6.0 )
      TS_ASSERT_DELTA( it->E(), sqrt(27.0), 0.00001 )
    }
	}

	void testInvalidInputs()
	{
	  MergeRuns merge2;
	  TS_ASSERT_THROWS_NOTHING( merge2.initialize() )
	  TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("OutputWorkspace","null") )
	  TS_ASSERT_THROWS( merge2.execute(), std::runtime_error )
    TS_ASSERT( ! merge2.isExecuted() )
    Workspace_sptr badIn = WorkspaceCreationHelper::Create2DWorkspace123(10,3,1);
	  badIn->dataX(0) = std::vector<double>(11,2.0);
    AnalysisDataService::Instance().add("badIn",badIn);
    TS_ASSERT_THROWS_NOTHING( merge.setPropertyValue("InputWorkspaces","ws1,badIn") )
    TS_ASSERT_THROWS( merge2.execute(), std::runtime_error )
    TS_ASSERT( ! merge2.isExecuted() )
	}

private:
  MergeRuns merge;
};

#endif /*MERGERUNSTEST_H_*/
