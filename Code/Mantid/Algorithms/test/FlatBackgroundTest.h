#ifndef FLATBACKGROUNDTEST_H_
#define FLATBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/FlatBackground.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidCurveFitting/Linear.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class FlatBackgroundTest : public CxxTest::TestSuite
{
public:
  FlatBackgroundTest()
  {
    bg = 100.0;
    const int numBins = 30;
    Mantid::DataObjects::Workspace1D_sptr WS(new Mantid::DataObjects::Workspace1D);
    WS->initialize(1,numBins+1,numBins);
    
    for (int i = 0; i < numBins; ++i)
    {
      WS->dataX(0)[i] = i;
      WS->dataY(0)[i] = bg+(static_cast<double>(std::rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2));
      WS->dataE(0)[i] = 0.05*WS->dataY(0)[i];   
    }
    WS->dataX(0)[numBins] = numBins;
    
    AnalysisDataService::Instance().add("flatBG",WS);
  }
  
	void testName()
	{
    TS_ASSERT_EQUALS( flatBG.name(), "FlatBackground" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( flatBG.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( flatBG.category(), "SANS" )
	}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( flatBG.initialize() )
    TS_ASSERT( flatBG.isInitialized() )

    const std::vector<Property*> props = flatBG.getProperties();
    TS_ASSERT_EQUALS( props.size(), 5 )
  }
  
  void testExec()
  {
    if ( !flatBG.isInitialized() ) flatBG.initialize();
    
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("InputWorkspace","flatBG") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("OutputWorkspace","Removed") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("WorkspaceIndexList","0") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("StartX","9.5") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("EndX","20.5") )

    TS_ASSERT_THROWS_NOTHING( flatBG.execute() )
    TS_ASSERT( flatBG.isExecuted() )
    
    MatrixWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("flatBG"));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Removed"));
    // The X vectors should be the same
    TS_ASSERT_EQUALS( inputWS->readX(0), outputWS->readX(0) )
    // Just do a spot-check on Y & E
    const std::vector<double> &Y = outputWS->readY(0);
    for (unsigned int i=0; i<Y.size(); ++i)
    {
      TS_ASSERT_LESS_THAN( Y[i], 1.5 )
    }
  }

private:
  Mantid::Algorithms::FlatBackground flatBG;
  double bg;
};

#endif /*FlatBackgroundTest_H_*/
