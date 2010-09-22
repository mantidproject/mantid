#ifndef DIFFRACTIONFOCUSSINGTEST_H_
#define DIFFRACTIONFOCUSSINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DiffractionFocussing.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class DiffractionFocussingTest : public CxxTest::TestSuite
{
public:
	void testName()
	{
		TS_ASSERT_EQUALS( focus.name(), "DiffractionFocussing" );
	}

	void testVersion()
	{
	  TS_ASSERT_EQUALS( focus.version(), 1 );
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( focus.category(), "Diffraction" );
	}

	void testInit()
	{
	  focus.initialize();
	  TS_ASSERT( focus.isInitialized() );
	}

	void testExec()
	{
    IAlgorithm* loader = new Mantid::DataHandling::LoadRaw;
    loader->initialize();
    loader->setPropertyValue("Filename", "../../../../Test/AutoTestData/HRP38692.RAW");

    std::string outputSpace = "tofocus";
    loader->setPropertyValue("OutputWorkspace", outputSpace);
    loader->setPropertyValue("SpectrumMin","50");
    loader->setPropertyValue("SpectrumMax","100");
    TS_ASSERT_THROWS_NOTHING( loader->execute() );
    TS_ASSERT( loader->isExecuted() );

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );
    focus.setPropertyValue("GroupingFileName","../../../../Test/AutoTestData/hrpd_new_072_01.cal");

	  TS_ASSERT_THROWS_NOTHING( focus.execute() );
	  TS_ASSERT( focus.isExecuted() );

		MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("focusedWS")) );

		// only 2 groups for this limited range of spectra
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 2 );
    
    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
    delete loader;
	}

private:
  DiffractionFocussing focus;
};

#endif /*DIFFRACTIONFOCUSSINGTEST_H_*/
