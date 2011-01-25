#ifndef FLATBACKGROUNDTEST_H_
#define FLATBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/FlatBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidCurveFitting/Linear.h"
#include <boost/lexical_cast.hpp>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;

static const int NUMBINS = 31;
static const int NUMSPECS = 4;

class FlatBackgroundTest : public CxxTest::TestSuite
{
public:
  static FlatBackgroundTest *createSuite() { return new FlatBackgroundTest(); }
  static void destroySuite(FlatBackgroundTest *suite) { delete suite; }

  FlatBackgroundTest()
  {
    bg = 100.0;
    Mantid::DataObjects::Workspace1D_sptr WS(new Mantid::DataObjects::Workspace1D);
    WS->initialize(1,NUMBINS+1,NUMBINS);
    
    for (int i = 0; i < NUMBINS; ++i)
    {
      WS->dataX(0)[i] = i;
      WS->dataY(0)[i] = bg+(static_cast<double>(std::rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2));
      WS->dataE(0)[i] = 0.05*WS->dataY(0)[i];   
    }
    WS->dataX(0)[NUMBINS] = NUMBINS;
    
    AnalysisDataService::Instance().add("flatBG",WS);

    //create another test wrokspace
    Mantid::DataObjects::Workspace2D_sptr WS2D(new Mantid::DataObjects::Workspace2D);
    WS2D->initialize(NUMSPECS,NUMBINS+1,NUMBINS);
    
    for (int j = 0; j < NUMSPECS; ++j)
    {
      for (int i = 0; i < NUMBINS; ++i)
      {
        WS2D->dataX(j)[i] = i;
        // any function that means the calculation is non-trival
        WS2D->dataY(j)[i] = j+4*(i+1)-(i*i)/10;
        WS2D->dataE(j)[i] = 2*i;
      }
      WS2D->dataX(j)[NUMBINS] = NUMBINS;
    }
    
    AnalysisDataService::Instance().add("flatbackgroundtest_ramp",WS2D);
  }
  
	void testStatics()
	{
    Mantid::Algorithms::FlatBackground flatBG;
    TS_ASSERT_EQUALS( flatBG.name(), "FlatBackground" )
    TS_ASSERT_EQUALS( flatBG.version(), 1 )
    TS_ASSERT_EQUALS( flatBG.category(), "SANS" )
  }

  void testExec()
  {
    Mantid::Algorithms::FlatBackground flatBG;
    TS_ASSERT_THROWS_NOTHING( flatBG.initialize() )
    TS_ASSERT( flatBG.isInitialized() )
    
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("InputWorkspace","flatBG") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("OutputWorkspace","Removed") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("WorkspaceIndexList","0") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("StartX","9.5") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("EndX","20.5") )
    TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("Mode","Linear Fit") )

    TS_ASSERT_THROWS_NOTHING( flatBG.execute() )
    TS_ASSERT( flatBG.isExecuted() )
    
    MatrixWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("flatBG"));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Removed"));
    // The X vectors should be the same
    TS_ASSERT_EQUALS( inputWS->readX(0), outputWS->readX(0) )
    // Just do a spot-check on Y & E
    const Mantid::MantidVec &Y = outputWS->readY(0);
    for (unsigned int i=0; i<Y.size(); ++i)
    {
      TS_ASSERT_LESS_THAN( Y[i], 1.5 )
    }
  }

  void testMeanFirst()
  {
    Mantid::Algorithms::FlatBackground back;
    TS_ASSERT_THROWS_NOTHING( back.initialize() )
    TS_ASSERT( back.isInitialized() )
    
    back.setPropertyValue("InputWorkspace","flatbackgroundtest_ramp");
    back.setPropertyValue("OutputWorkspace","flatbackgroundtest_first");
    back.setPropertyValue("WorkspaceIndexList","");
    back.setPropertyValue("Mode","Mean");
    // remove the first half of the spectrum
    back.setPropertyValue("StartX","0");
    back.setPropertyValue("EndX","15");

    TS_ASSERT_THROWS_NOTHING( back.execute() )
    TS_ASSERT( back.isExecuted() )
    
    MatrixWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("flatbackgroundtest_ramp"));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("flatbackgroundtest_first"));
    // The X vectors should be the same
    TS_ASSERT_EQUALS( inputWS->readX(0), outputWS->readX(0) )

    for (int j = 0; j < NUMSPECS; ++j)
    {
      const Mantid::MantidVec &YIn = inputWS->readY(j);
      const Mantid::MantidVec &EIn = inputWS->readE(j);
      const Mantid::MantidVec &YOut = outputWS->readY(j);
      const Mantid::MantidVec &EOut = outputWS->readE(j);
      // do our own calculation of the background and its error to check with later
      double background = 0, backError = 0;
      for( int k = 0; k < 15; ++k)
      {
        background += YIn[k];
        backError += EIn[k]*EIn[k];
      }
      background /= 15.0;
      backError = std::sqrt(backError)/15.0;
      for (int i = 0; i < NUMBINS; ++i)
      {
        double correct = ( YIn[i] - background ) > 0 ? YIn[i]-background : 0;
        TS_ASSERT_EQUALS( YOut[i], correct )

        if ( YIn[i] - background < 0 )
        {
          TS_ASSERT_EQUALS(EOut[i], background)
        }
        else
        {
          TS_ASSERT_EQUALS(EOut[i], std::sqrt((EIn[i]*EIn[i])+(backError*backError)) )
        }
      }
    }
  }

  void testMeanSecond()
  {
    Mantid::Algorithms::FlatBackground back;
    TS_ASSERT_THROWS_NOTHING( back.initialize() )
    TS_ASSERT( back.isInitialized() )
    
    back.setPropertyValue("InputWorkspace","flatbackgroundtest_ramp");
    back.setPropertyValue("OutputWorkspace","flatbackgroundtest_second");
    back.setPropertyValue("WorkspaceIndexList","");
    back.setPropertyValue("Mode","Mean");
    // remove the last half of the spectrum
    back.setProperty("StartX", 2*double(NUMBINS)/3);
    back.setProperty("EndX", double(NUMBINS));

    TS_ASSERT_THROWS_NOTHING( back.execute() )
    TS_ASSERT( back.isExecuted() )
    
    MatrixWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("flatbackgroundtest_ramp"));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("flatbackgroundtest_second"));
    // The X vectors should be the same
    TS_ASSERT_EQUALS( inputWS->readX(0), outputWS->readX(0) )

    for (int j = 0; j < NUMSPECS; ++j)
    {
      const Mantid::MantidVec &YIn = inputWS->readY(j);
      const Mantid::MantidVec &EIn = inputWS->readE(j);
      const Mantid::MantidVec &YOut = outputWS->readY(j);
      const Mantid::MantidVec &EOut = outputWS->readE(j);
      // do our own calculation of the background and its error to check with later
      double background = 0, backError = 0, numSummed = 0;
      // 2*NUMBINS/3 makes use of the truncation of integer division
      for( int k = 2*NUMBINS/3; k < NUMBINS; ++k)
      {
        background += YIn[k];
        backError += EIn[k]*EIn[k];
        numSummed++;
      }
      background /= numSummed ;
      backError = std::sqrt(backError)/numSummed ;
      for (int i = 0; i < NUMBINS; ++i)
      {
        double correct = ( YIn[i] - background ) > 0 ? YIn[i]-background : 0;
        TS_ASSERT_EQUALS( YOut[i], correct )

        if ( YIn[i] - background < 0 && EIn[i] < background )
        {
          TS_ASSERT_EQUALS(EOut[i], background)
        }
        else
        {
          TS_ASSERT_EQUALS(EOut[i], std::sqrt((EIn[i]*EIn[i])+(backError*backError)) )
        }
      }
    }
  }

private:
  double bg;
};

#endif /*FlatBackgroundTest_H_*/
