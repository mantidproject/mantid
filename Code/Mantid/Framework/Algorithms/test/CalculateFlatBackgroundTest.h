#ifndef FLATBACKGROUNDTEST_H_
#define FLATBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CalculateFlatBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/Linear.h"
#include "MantidKernel/MersenneTwister.h"
#include <boost/lexical_cast.hpp>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;

static const int NUMBINS = 31;
static const int NUMSPECS = 4;

class CalculateFlatBackgroundTest : public CxxTest::TestSuite
{
  ///Tests each method in CalculateFlatBackground using different parameter sets to make sure the returns are as expected
private:
  ///Run CalculateFlatBackground with options specific to the function calling it. Each function has a preset relating to that specific test's needs.
  ///@param functindex - an integer identifying the function running the algorithm in order to set the properties
  void runCalculateFlatBackground(int functindex)
  {
    // functindex key
    // 1 = exec
    // 2 = execwithreturnbg
    // 3 = testMeanFirst
    // 4 = testMeanFirstWithReturnBackground
    // 5 = testMeanSecond

    //common beginning
    Mantid::Algorithms::CalculateFlatBackground flatBG;
    TS_ASSERT_THROWS_NOTHING( flatBG.initialize() )
    TS_ASSERT( flatBG.isInitialized() )

    if (functindex == 1 || functindex == 2)
    {
      //exec or execWithReturnBackground
      TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("InputWorkspace","calcFlatBG") )
      TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("OutputWorkspace","Removed") )
      TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("WorkspaceIndexList","0") )
      TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("StartX","9.5") )
      TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("EndX","20.5") )
      TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("Mode","Linear Fit") )

      if(functindex == 2)
      {
        //execWithReturnBackground
        TS_ASSERT_THROWS_NOTHING( flatBG.setPropertyValue("OutputMode","Return Background") )
      }
    }
    else if (functindex == 3 || functindex == 4 || functindex == 5)
    {
      flatBG.setPropertyValue("InputWorkspace","calculateflatbackgroundtest_ramp");
      flatBG.setPropertyValue("WorkspaceIndexList","");
      flatBG.setPropertyValue("Mode","Mean");

      if (functindex == 3 || functindex == 4)
      {
        //testMeanFirst or testMeanFirstWithReturnBackground
        flatBG.setPropertyValue("OutputWorkspace","calculateflatbackgroundtest_first");
        // remove the first half of the spectrum
        flatBG.setPropertyValue("StartX","0");
        flatBG.setPropertyValue("EndX","15");
        if (functindex == 4)
        {
          //testMeanFirstWithReturnBackground
          flatBG.setPropertyValue("OutputMode","Return Background"); 
        }
      }
      else if (functindex == 5)
      {
        //testMeanSecond
        flatBG.setPropertyValue("OutputWorkspace","calculateflatbackgroundtest_second");
        // remove the last half of the spectrum
        flatBG.setProperty("StartX", 2*double(NUMBINS)/3);
        flatBG.setProperty("EndX", double(NUMBINS));
      }
    }

    // common ending
    TS_ASSERT_THROWS_NOTHING( flatBG.execute() )
    TS_ASSERT( flatBG.isExecuted() )
  }

public:
  static CalculateFlatBackgroundTest *createSuite() { return new CalculateFlatBackgroundTest(); }
  static void destroySuite(CalculateFlatBackgroundTest *suite) { delete suite; }

  CalculateFlatBackgroundTest()
  {
    bg = 100.0;
    Mantid::DataObjects::Workspace2D_sptr WS(new Mantid::DataObjects::Workspace2D);
    WS->initialize(1,NUMBINS+1,NUMBINS);
    const size_t seed(12345);
    const double lower(-1.0), upper(1.0);
    MersenneTwister randGen(seed, lower, upper);
    
    for (int i = 0; i < NUMBINS; ++i)
    {
      WS->dataX(0)[i] = i;
      WS->dataY(0)[i] = bg + randGen.nextValue();
      WS->dataE(0)[i] = 0.05*WS->dataY(0)[i];   
    }
    WS->dataX(0)[NUMBINS] = NUMBINS;
    
    AnalysisDataService::Instance().add("calcFlatBG",WS);

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
    
    AnalysisDataService::Instance().add("calculateflatbackgroundtest_ramp",WS2D);
  }

  ~CalculateFlatBackgroundTest()
  {
    AnalysisDataService::Instance().remove("calculateflatbackgroundtest_ramp");
  }
  
	void testStatics()
	{
    Mantid::Algorithms::CalculateFlatBackground flatBG;
    TS_ASSERT_EQUALS( flatBG.name(), "CalculateFlatBackground" )
    TS_ASSERT_EQUALS( flatBG.version(), 1 )
  }

  void testExec()
  {
    runCalculateFlatBackground(1);

    MatrixWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calcFlatBG");
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Removed");
    // The X vectors should be the same
    TS_ASSERT_DELTA( inputWS->readX(0), outputWS->readX(0) , 1e-6 )
    // Just do a spot-check on Y & E
    const Mantid::MantidVec &Y = outputWS->readY(0);
    for (unsigned int i=0; i<Y.size(); ++i)
    {
      TS_ASSERT_LESS_THAN( Y[i], 1.5 )
    }
  }

  void testExecWithReturnBackground()
  {
    runCalculateFlatBackground(2);

    MatrixWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calcFlatBG");
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Removed");
    // The X vectors should be the same
    TS_ASSERT_DELTA( inputWS->readX(0), outputWS->readX(0) , 1e-6 )
    // Just do a spot-check on Y & E
    const Mantid::MantidVec &Y = outputWS->readY(0);
    for (unsigned int i=0; i<Y.size(); ++i)
    {
      TS_ASSERT( 100.3431 > Y[i]);
    }
  }

  void testMeanFirst()
  {
    runCalculateFlatBackground(3);

    MatrixWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_first");
    // The X vectors should be the same
    TS_ASSERT_DELTA( inputWS->readX(0), outputWS->readX(0) , 1e-6 )

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
        TS_ASSERT_DELTA( YOut[i], correct , 1e-6 )

        if ( YIn[i] - background < 0 )
        {
          TS_ASSERT_DELTA(EOut[i], background, 1e-6 )
        }
        else
        {
          TS_ASSERT_DELTA(EOut[i], std::sqrt((EIn[i]*EIn[i])+(backError*backError)), 1e-6 )
        }
      }
    }
  }

  void testMeanFirstWithReturnBackground()
  {
    runCalculateFlatBackground(4);

    MatrixWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_first");
    // The X vectors should be the same
    TS_ASSERT_DELTA( inputWS->readX(0), outputWS->readX(0) , 1e-6 )

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
        TS_ASSERT_DELTA(YOut[i], background, 1e-6 )
        TS_ASSERT_DELTA(EOut[i], std::sqrt((EIn[i]*EIn[i])+(backError*backError)), 1e-6 )
      }
    }
  }

  void testMeanSecond()
  {
    runCalculateFlatBackground(5);

    MatrixWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_second");
    // The X vectors should be the same
    TS_ASSERT_DELTA( inputWS->readX(0), outputWS->readX(0) , 1e-6 )

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
        TS_ASSERT_DELTA( YOut[i], correct  , 1e-6 )

        if ( YIn[i] - background < 0 && EIn[i] < background )
        {
          TS_ASSERT_DELTA(EOut[i], background , 1e-6 )
        }
        else
        {
          TS_ASSERT_DELTA(EOut[i], std::sqrt((EIn[i]*EIn[i])+(backError*backError))  , 1e-6 )
        }
      }
    }
  }

  void testVariedWidths()
  {
    const double YVALUE = 100.0;
    Mantid::DataObjects::Workspace2D_sptr WS(new Mantid::DataObjects::Workspace2D);
    WS->initialize(1,NUMBINS+1,NUMBINS);
    
    for (int i = 0; i < NUMBINS; ++i)
    {
      WS->dataX(0)[i] = 2*i;
      WS->dataY(0)[i] = YVALUE;
      WS->dataE(0)[i] = YVALUE/3.0;
    }
    WS->dataX(0)[NUMBINS] = 2*(NUMBINS-1)+4.0;
    

    Mantid::Algorithms::CalculateFlatBackground back;
    back.initialize();
    
    back.setProperty("InputWorkspace", boost::static_pointer_cast<Mantid::API::MatrixWorkspace>(WS));
    back.setPropertyValue("OutputWorkspace","calculateflatbackgroundtest_third");
    back.setPropertyValue("WorkspaceIndexList","");
    back.setPropertyValue("Mode","Mean");
    // sample the background from the last (wider) bin only
    back.setProperty("StartX", 2.0*NUMBINS+1);
    back.setProperty("EndX", 2.0*(NUMBINS+1));

    back.execute();
    TS_ASSERT( back.isExecuted() )
    
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("calculateflatbackgroundtest_third");
    // The X vectors should be the same
    TS_ASSERT_DELTA( WS->readX(0), outputWS->readX(0) , 1e-6 )

    const Mantid::MantidVec &YOut = outputWS->readY(0);
    const Mantid::MantidVec &EOut = outputWS->readE(0);

    TS_ASSERT_DELTA( YOut[5], 50.0  , 1e-6 )
    TS_ASSERT_DELTA( YOut[25], 50.0  , 1e-6 )
    TS_ASSERT_DELTA( YOut[NUMBINS-1], 0.0  , 1e-6 )

    TS_ASSERT_DELTA( EOut[10], 37.2677, 0.001 )
    TS_ASSERT_DELTA( EOut[20], 37.2677, 0.001 )
  }
  
private:
  double bg;

  double round( double value )
  {
    return floor( value*100000 + 0.5 )/100000;
  }
};

#endif /*FlatBackgroundTest_H_*/
