#ifndef ALIGNDETECTORSTEST_H_
#define ALIGNDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/AlignDetectors.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class AlignDetectorsTest : public CxxTest::TestSuite
{
public:
  AlignDetectorsTest()
  {
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/HRP38692.RAW");
    inputWS = "rawWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();
  }

	void testName()
	{
	  TS_ASSERT_EQUALS( align.name(), "AlignDetectors" )
	}

	void testVersion()
	{
	  TS_ASSERT_EQUALS( align.version(), 1 )
	}

	void testCategory()
	{
	  TS_ASSERT_EQUALS( align.category(), "DataHandling\\Detectors" )
	}

	void testInit()
	{
	  TS_ASSERT_THROWS_NOTHING( align.initialize() )
	  TS_ASSERT( align.isInitialized() )

	  std::vector<Property*> props = align.getProperties();
	  TS_ASSERT_EQUALS( props.size(), 3 )
	}

	void testExec()
	{
	  if ( !align.isInitialized() ) align.initialize();

	  TS_ASSERT_THROWS( align.execute(), std::runtime_error )

	  align.setPropertyValue("InputWorkspace", inputWS);
	  align.setPropertyValue("OutputWorkspace", "aligned");
	  align.setPropertyValue("CalibrationFile", "../../../../Test/Data/hrpd_new_072_01.cal");

	  TS_ASSERT_THROWS_NOTHING( align.execute() )
	  TS_ASSERT( align.isExecuted() )
	}

private:
  AlignDetectors align;
  std::string inputWS;

};

#endif /*ALIGNDETECTORSTEST_H_*/
