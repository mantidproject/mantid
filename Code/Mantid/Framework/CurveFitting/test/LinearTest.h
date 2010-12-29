#ifndef LINEARTEST_H_
#define LINEARTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/Linear.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class LinearTest : public CxxTest::TestSuite
{
public:
  LinearTest()
  {
    c0 = 10.0;
    c1 = 1.0;
    const int numBins = 30;
    Mantid::DataObjects::Workspace1D_sptr WS(new Mantid::DataObjects::Workspace1D);
    WS->initialize(1,numBins+1,numBins);
    
    for (int i = 0; i < numBins; ++i)
    {
      WS->dataX(0)[i] = i;
      WS->dataY(0)[i] = (c0 + c1*i)+(static_cast<double>(std::rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2));
      WS->dataE(0)[i] = 0.05*WS->dataY(0)[i];   
    }
    WS->dataX(0)[numBins] = numBins;

    // Mask out a couple of bins to test that functionality
    WS->maskBin(0,15);
    WS->maskBin(0,21,0.5);
    
    AnalysisDataService::Instance().add("Line",WS);
  }
  
  void testName()
  {
    TS_ASSERT_EQUALS( lin.name(), "Linear" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( lin.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( lin.category(), "CurveFitting" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( lin.initialize() )
    TS_ASSERT( lin.isInitialized() )

    const std::vector<Property*> props = lin.getProperties();
    TS_ASSERT_EQUALS( props.size(), 9 )
  }
  
  void testExec()
  {
    if ( !lin.isInitialized() ) lin.initialize();
    
    TS_ASSERT_THROWS_NOTHING( lin.setPropertyValue("InputWorkspace","Line") )
    TS_ASSERT_THROWS_NOTHING( lin.setPropertyValue("OutputWorkspace","Fit") )

    TS_ASSERT_THROWS_NOTHING( lin.execute() )
    TS_ASSERT( lin.isExecuted() )

    TS_ASSERT_EQUALS( lin.getPropertyValue("FitStatus"), "success" )
    const double intercept = lin.getProperty("FitIntercept");
    TS_ASSERT_DELTA( intercept, c0, 0.1*c0 )
    const double slope = lin.getProperty("FitSlope");
    TS_ASSERT_DELTA( slope, c1, 0.1*c1 )
    const double chisq = lin.getProperty("Chi^2");
    TS_ASSERT( chisq )
    
    MatrixWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Line"));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Fit"));
    // The X vectors should be the same
    TS_ASSERT_EQUALS( inputWS->readX(0), outputWS->readX(0) )
    // Just do a spot-check on Y & E
    TS_ASSERT( outputWS->readY(0)[10] )
  }

private:
  Mantid::CurveFitting::Linear lin;
  double c0,c1;
};

#endif /*LINEARTEST_H_*/
