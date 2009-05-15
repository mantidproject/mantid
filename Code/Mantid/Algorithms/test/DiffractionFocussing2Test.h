#ifndef DIFFRACTIONFOCUSSING2TEST_H_
#define DIFFRACTIONFOCUSSING2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/AlignDetectors.h"
#include "MantidAlgorithms/MaskBins.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class DiffractionFocussing2Test : public CxxTest::TestSuite
{
public:
	void testName()
	{
		TS_ASSERT_EQUALS( focus.name(), "DiffractionFocussing" )
	}

	void testVersion()
	{
	  TS_ASSERT_EQUALS( focus.version(), 2 )
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
    Mantid::DataHandling::LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/Data/HRP38692.RAW");

    std::string outputSpace = "tofocus";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.setPropertyValue("spectrum_min","50");
    loader.setPropertyValue("spectrum_max","100");
    TS_ASSERT_THROWS_NOTHING( loader.execute() )
    TS_ASSERT( loader.isExecuted() )
    
    // Have to align because diffraction focussing wants d-spacing
    Mantid::DataHandling::AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace",outputSpace);
    align.setPropertyValue("OutputWorkspace",outputSpace);
    align.setPropertyValue("CalibrationFile","../../../../Test/Data/hrpd_new_072_01.cal");
    TS_ASSERT_THROWS_NOTHING( align.execute() )
    TS_ASSERT( align.isExecuted() )

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );
    focus.setPropertyValue("GroupingFileName","../../../../Test/Data/hrpd_new_072_01.cal");

	  TS_ASSERT_THROWS_NOTHING( focus.execute() )
	  TS_ASSERT( focus.isExecuted() )

		MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("focusedWS")) )

		// only 2 groups for this limited range of spectra
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 2 )
    
    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
	}

private:
  DiffractionFocussing2 focus;
};

#endif /*DIFFRACTIONFOCUSSING2TEST_H_*/
