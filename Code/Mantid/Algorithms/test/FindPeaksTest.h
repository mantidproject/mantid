#ifndef FINDPEAKSTEST_H_
#define FINDPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/FindPeaks.h"
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidCurveFitting/GaussianLinearBG.h"

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
    std::string outputWS("stripped");
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("SmoothedData","smoothed") )
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("WithPeaksStripped",outputWS) )

    TS_ASSERT_THROWS_NOTHING( finder.execute() )
    TS_ASSERT( finder.isExecuted() )
  }

private:
  FindPeaks finder;
};

#endif /*FINDPEAKSTEST_H_*/
