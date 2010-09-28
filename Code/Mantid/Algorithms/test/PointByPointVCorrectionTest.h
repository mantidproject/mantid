#ifndef POINTBYPOINTVCORRECTIONTEST_H_
#define POINTBYPOINTVCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PointByPointVCorrection.h"
#include "MantidDataHandling/LoadRaw3.h"

class PointByPointVCorrectionTest : public CxxTest::TestSuite
{
public:
	void testName()
  {
	  TS_ASSERT_EQUALS( pbpv.name(), "PointByPointVCorrection")
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( pbpv.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( pbpv.category(), "Diffraction" )
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( pbpv.initialize() )
    TS_ASSERT( pbpv.isInitialized() )
  }
  
  void testExec()
  {
    using namespace Mantid::API;
    Mantid::DataHandling::LoadRaw3 loader1;
    loader1.initialize();
    loader1.setPropertyValue("Filename", "../../../../Test/AutoTestData/HRP39182.raw");
    loader1.setPropertyValue("OutputWorkspace", "sample");
    loader1.setPropertyValue("SpectrumMin","1");
    loader1.setPropertyValue("SpectrumMax","10");
    loader1.execute();
    
    Mantid::DataHandling::LoadRaw3 loader2;
    loader2.initialize();
    loader2.setPropertyValue("Filename", "../../../../Test/AutoTestData/HRP39191.raw");
    loader2.setPropertyValue("OutputWorkspace", "vanadium");
    loader2.setPropertyValue("SpectrumMin","1");
    loader2.setPropertyValue("SpectrumMax","10");
    loader2.execute();
    
    if ( !pbpv.isInitialized() ) pbpv.initialize();
    
    pbpv.setPropertyValue("InputW1","sample");
    pbpv.setPropertyValue("InputW2","vanadium");
    pbpv.setPropertyValue("OutputWorkspace","out");
    TS_ASSERT_THROWS_NOTHING( pbpv.execute() )
    TS_ASSERT( pbpv.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("out")) )

    // Check a few values
    TS_ASSERT_DELTA( output->readY(6)[9623], 2.9667, 0.0001 )
    TS_ASSERT_DELTA( output->readY(4)[23161], 55.1607, 0.0001 )
    TS_ASSERT_DELTA( output->readY(0)[0], 0, 0.000001 )
    TS_ASSERT_DELTA( output->readE(9)[15909], 1.3791, 0.0001 )
    TS_ASSERT_DELTA( output->readE(2)[5513], 0.4299, 0.0001 )
    TS_ASSERT_DELTA( output->readE(0)[0], 0, 0.000001 )

    AnalysisDataService::Instance().remove("sample");
    AnalysisDataService::Instance().remove("vanadium");
    AnalysisDataService::Instance().remove("out");
  }

private:
  Mantid::Algorithms::PointByPointVCorrection pbpv;
};

#endif /*POINTBYPOINTVCORRECTIONTEST_H_*/
