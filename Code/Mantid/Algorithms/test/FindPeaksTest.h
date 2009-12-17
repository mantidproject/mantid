#ifndef FINDPEAKSTEST_H_
#define FINDPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/FindPeaks.h"
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidCurveFitting/GaussianLinearBG1D.h"

using namespace Mantid::API;
using Mantid::Algorithms::FindPeaks;

class FindPeaksTest : public CxxTest::TestSuite
{
public:
  FindPeaksTest()
  {
    Mantid::NeXus::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","../../../../Test/Nexus/focussed.nxs");
    loader.setProperty("OutputWorkspace","peaksWS");
    loader.execute();
  }

	void testName()
	{
    TS_ASSERT_EQUALS( finder.name(), "FindPeaks" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( finder.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( finder.category(), "General" )
	}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( finder.initialize() )
    TS_ASSERT( finder.isInitialized() )
  }

  void testExec()
  {
    if ( !finder.isInitialized() ) finder.initialize();

    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("InputWorkspace","peaksWS") )
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("WorkspaceIndex","4") )
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("SmoothedData","smoothed") )
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("PeaksList","foundpeaks") )

    TS_ASSERT_THROWS_NOTHING( finder.execute() )
    TS_ASSERT( finder.isExecuted() )

    ITableWorkspace_sptr peaklist = boost::dynamic_pointer_cast<ITableWorkspace>
                                     (AnalysisDataService::Instance().retrieve("foundpeaks"));
    TS_ASSERT( peaklist )
    TS_ASSERT_EQUALS( peaklist->rowCount() , 8 )
    TS_ASSERT_DELTA( peaklist->Double(0,1), 0.59, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(1,1), 0.71, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(2,1), 0.81, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(3,1), 1.03, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(4,1), 0.96, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(5,1), 1.24, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(6,1), 1.52, 0.01 )
    TS_ASSERT_DELTA( peaklist->Double(7,1), 2.14, 0.01 )
  }

private:
  FindPeaks finder;
};

#endif /*FINDPEAKSTEST_H_*/
