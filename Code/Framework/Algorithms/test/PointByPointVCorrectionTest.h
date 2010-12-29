#ifndef POINTBYPOINTVCORRECTIONTEST_H_
#define POINTBYPOINTVCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PointByPointVCorrection.h"
#include "WorkspaceCreationHelper.hh"

class PointByPointVCorrectionTest : public CxxTest::TestSuite
{
public:
	void testName()
  {
	  TS_ASSERT_EQUALS( pbpv.name(), "PointByPointVCorrection");
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( pbpv.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( pbpv.category(), "Diffraction" );
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( pbpv.initialize() );
    TS_ASSERT( pbpv.isInitialized() );
  }
  
  void testExec()
  {
    using namespace Mantid::API;
    
    if ( !pbpv.isInitialized() ) pbpv.initialize();
    
    MatrixWorkspace_sptr testSample = WorkspaceCreationHelper::Create2DWorkspaceBinned(2,5,0.5,1.5);
    MatrixWorkspace_sptr testVanadium = WorkspaceCreationHelper::Create2DWorkspaceBinned(2,5,0.5,1.5);
    // Make the instruments match
    testVanadium->setInstrument(testSample->getBaseInstrument());
    // Change the Y values
    testSample->dataY(1) = Mantid::MantidVec(5,3.0);
    testVanadium->dataY(1) = Mantid::MantidVec(5,5.5);
    
    pbpv.setProperty<MatrixWorkspace_sptr>("InputW1",testSample);
    pbpv.setProperty<MatrixWorkspace_sptr>("InputW2",testVanadium);
    pbpv.setPropertyValue("OutputWorkspace","out");
    TS_ASSERT_THROWS_NOTHING( pbpv.execute() );
    TS_ASSERT( pbpv.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("out")) );

    // Check a few values
    TS_ASSERT_DELTA( output->readY(1)[4], 2.9999, 0.0001 );
    TS_ASSERT_DELTA( output->readY(1)[1], 2.9999, 0.0001 );
    TS_ASSERT_DELTA( output->readY(0)[0], 2.0, 0.000001 );
    TS_ASSERT_DELTA( output->readE(1)[3], 1.8745, 0.0001 );
    TS_ASSERT_DELTA( output->readE(1)[2], 1.8745, 0.0001 );
    TS_ASSERT_DELTA( output->readE(0)[0], 2.2803, 0.0001 );

    AnalysisDataService::Instance().remove("out");
  }

private:
  Mantid::Algorithms::PointByPointVCorrection pbpv;
};

#endif /*POINTBYPOINTVCORRECTIONTEST_H_*/
