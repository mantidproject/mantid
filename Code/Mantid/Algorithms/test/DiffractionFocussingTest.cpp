#ifndef DIFFRACTIONFOCUSSINGTEST_H_
#define DIFFRACTIONFOCUSSINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DiffractionFocussing.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;

class DiffractionFocussingTest : public CxxTest::TestSuite
{
public:
	void testName()
	{
		TS_ASSERT_EQUALS( focus.name(), "DiffractionFocussing" )
	}

	void testVersion()
	{
	  TS_ASSERT_EQUALS( focus.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( focus.category(), "Diffraction" )
	}

	void testInit()
	{
	  focus.initialize();
	  TS_ASSERT( focus.isInitialized() )
	}

	void testExec()
	{
    IAlgorithm* loader = FrameworkManager::Instance().createAlgorithm("LoadRaw");
    loader->setPropertyValue("Filename", "../../../../Test/Data/HRP38692.RAW");

    std::string outputSpace = "tofocus";
    loader->setPropertyValue("OutputWorkspace", outputSpace);
    loader->setPropertyValue("spectrum_min","1");
    loader->setPropertyValue("spectrum_max","10");
    loader->execute();
    TS_ASSERT( loader->isExecuted() )

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );
    focus.setPropertyValue("GroupingFileName","../../../../Test/Data/hrpd_new_072_01.cal");

	  focus.execute();
	  TS_ASSERT( focus.isExecuted() )
	}

private:
  DiffractionFocussing focus;
};

#endif /*DIFFRACTIONFOCUSSINGTEST_H_*/
