#ifndef STRIPPEAKSTEST_H_
#define STRIPPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/StripPeaks.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidCurveFitting/Gaussian.h"

using namespace Mantid::API;
using Mantid::Algorithms::StripPeaks;

class StripPeaksTest : public CxxTest::TestSuite
{
public:
  StripPeaksTest()
  {
    Workspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(2,100,0.5,0.02);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    AnalysisDataService::Instance().add("toStrip",WS);
  }

	void testName()
	{
    TS_ASSERT_EQUALS( strip.name(), "StripPeaks" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( strip.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( strip.category(), "General" )
	}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( strip.initialize() )
    TS_ASSERT( strip.isInitialized() )
  }

  void testExec()
  {
    if ( !strip.isInitialized() ) strip.initialize();

    TS_ASSERT_THROWS_NOTHING( strip.setPropertyValue("InputWorkspace","toStrip") )
    std::string outputWS("stripped");
    TS_ASSERT_THROWS_NOTHING( strip.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( strip.execute() )
    TS_ASSERT( strip.isExecuted() )

    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve(outputWS) )
  }

private:
  StripPeaks strip;
};

#endif /*STRIPPEAKSTEST_H_*/
