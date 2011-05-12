#ifndef GETDETECTOROFFSETSTEST_H_
#define GETDETECTOROFFSETSTEST_H_

#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/GaussianLinearBG1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;
using Mantid::Algorithms::GetDetectorOffsets;
using Mantid::DataObjects::OffsetsWorkspace_sptr;

class GetDetectorOffsetsTest : public CxxTest::TestSuite
{
public:
  GetDetectorOffsetsTest()
  {
  }

  void testTheBasics()
  {
    TS_ASSERT_EQUALS( offsets.name(), "GetDetectorOffsets" );
    TS_ASSERT_EQUALS( offsets.version(), 1 );
    TS_ASSERT_EQUALS( offsets.category(), "Diffraction" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( offsets.initialize() );
    TS_ASSERT( offsets.isInitialized() );
  }

  void testExec()
  {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1,200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    const Mantid::MantidVec &X = WS->readX(0);
    Mantid::MantidVec &Y = WS->dataY(0);
    Mantid::MantidVec &E = WS->dataE(0);
    for (int i = 0; i < Y.size(); ++i)
    {
      const double x = (X[i]+X[i+1])/2;
      Y[i] = exp(-0.5*pow((x-1)/10.0,2));
      E[i] = 0.001;
    }

    // ---- Run algo -----
    if ( !offsets.isInitialized() ) offsets.initialize();
    TS_ASSERT_THROWS_NOTHING( offsets.setProperty("InputWorkspace",WS) );
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step","0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference","1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin","-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax","20"));
    TS_ASSERT_THROWS_NOTHING( offsets.execute() );
    TS_ASSERT( offsets.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) );
    if (!output) return;

    TS_ASSERT_DELTA( output->dataY(0)[0], -0.0196, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  GetDetectorOffsets offsets;
};

class GetDetectorOffsetsTestPerformance : public CxxTest::TestSuite
{
public:
  MatrixWorkspace_sptr WS;
  int numpixels;

  void setUp()
  {
    numpixels = 10000;
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numpixels,200, false);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    for (int wi=0; wi<WS->getNumberHistograms(); wi++)
    {
      const Mantid::MantidVec &X = WS->readX(wi);
      Mantid::MantidVec &Y = WS->dataY(wi);
      Mantid::MantidVec &E = WS->dataE(wi);
      for (int i = 0; i < static_cast<int>(Y.size()); ++i)
      {
        const double x = (X[i]+X[i+1])/2;
        Y[i] = exp(-0.5*pow((x-1)/10.0,2));
        E[i] = 0.001;
      }
    }
  }

  void test_performance()
  {
    AlgorithmManager::Instance(); //Initialize here to avoid an odd ABORT
    GetDetectorOffsets offsets;
    if ( !offsets.isInitialized() ) offsets.initialize();
    TS_ASSERT_THROWS_NOTHING( offsets.setProperty("InputWorkspace", WS) );
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("Step","0.02"));
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("DReference","1.00"));
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("XMin","-20"));
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("XMax","20"));
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("OutputWorkspace","dummyname"));
    TS_ASSERT_THROWS_NOTHING( offsets.execute() );
    TS_ASSERT( offsets.isExecuted() );
    OffsetsWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = offsets.getProperty("OutputWorkspace") );
    if (!output) return;
    TS_ASSERT_DELTA( output->dataY(0)[0], -0.0196, 0.0001);
  }

};

#endif /*GETDETECTOROFFSETSTEST_H_*/
