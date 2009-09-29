#ifndef SMOOTHDATATEST_H_
#define SMOOTHDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SmoothData.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class SmoothDataTest : public CxxTest::TestSuite
{
public:
  SmoothDataTest()
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",2,10,10);
    for (int i = 0; i < 10; ++i)
    {
      space->dataY(0)[i] = i+1.0;
      space->dataE(0)[i] = sqrt(i+1.0);
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("noisy", space);
  }

  void testName()
  {
    TS_ASSERT_EQUALS( smooth.name(), "SmoothData" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( smooth.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( smooth.category(), "General" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( smooth.initialize() )
    TS_ASSERT( smooth.isInitialized() )
  }

  void testInvalidInputs()
  {
    SmoothData smooth2;
    TS_ASSERT_THROWS_NOTHING( smooth2.initialize() )
    TS_ASSERT_THROWS( smooth2.execute(), std::runtime_error )
    // Can't set Npoints to value less than 3
    TS_ASSERT_THROWS( smooth2.setPropertyValue("NPoints","1"), std::invalid_argument )

    TS_ASSERT_THROWS_NOTHING( smooth2.setPropertyValue("InputWorkspace","noisy") )
    TS_ASSERT_THROWS_NOTHING( smooth2.setPropertyValue("OutputWorkspace","something") )
    // Will also fail if NPoints is larger than spectrum length
    TS_ASSERT_THROWS_NOTHING( smooth2.setPropertyValue("NPoints","11") )
    TS_ASSERT_THROWS_NOTHING( smooth2.execute() )
    TS_ASSERT( ! smooth2.isExecuted() )
  }

  void testExec()
  {
    if ( !smooth.isInitialized() ) smooth.initialize();

    TS_ASSERT_THROWS_NOTHING( smooth.setPropertyValue("InputWorkspace","noisy") )
    std::string outputWS("smoothed");
    TS_ASSERT_THROWS_NOTHING( smooth.setPropertyValue("OutputWorkspace",outputWS) )
    // Set to 4 - algorithm should change it to 5
    TS_ASSERT_THROWS_NOTHING( smooth.setPropertyValue("NPoints","4") )

    TS_ASSERT_THROWS_NOTHING( smooth.execute() )
    TS_ASSERT( smooth.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) )
    const Mantid::MantidVec &Y = output->dataY(0);
    const Mantid::MantidVec &E = output->dataE(0);
    TS_ASSERT_EQUALS( Y[0], 2 )
    TS_ASSERT_DELTA( E[0], sqrt(Y[0]/3.0), 0.0001 )
    TS_ASSERT_EQUALS( Y[1], 2.5 )
    TS_ASSERT_DELTA( E[1], sqrt(Y[1]/4.0), 0.0001 )
    for (int i = 2; i < output->blocksize()-2; ++i)
    {
      TS_ASSERT_EQUALS( Y[i], i+1.0 )
      TS_ASSERT_DELTA( E[i], sqrt(Y[i]/5.0), 0.0001 )
    }
    TS_ASSERT_EQUALS( Y[8], 8.5 )
    TS_ASSERT_DELTA( E[8], sqrt(Y[8]/4.0), 0.0001 )
    TS_ASSERT_EQUALS( Y[9], 9 )
    TS_ASSERT_DELTA( E[9], sqrt(Y[9]/3.0), 0.0001 )

    // Check X vectors are shared
    TS_ASSERT_EQUALS( &(output->dataX(0)), &(output->dataX(1)) )

    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  SmoothData smooth;
};

#endif /*SMOOTHDATATEST_H_*/
