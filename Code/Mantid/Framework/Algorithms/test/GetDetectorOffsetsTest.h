#ifndef GETDETECTOROFFSETSTEST_H_
#define GETDETECTOROFFSETSTEST_H_

#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/GaussianLinearBG1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>

using namespace Mantid::API;
using Mantid::Algorithms::GetDetectorOffsets;

class GetDetectorOffsetsTest : public CxxTest::TestSuite
{
public:
  static GetDetectorOffsetsTest *createSuite() { return new GetDetectorOffsetsTest(); }
  static void destroySuite(GetDetectorOffsetsTest *suite) { delete suite; }

  GetDetectorOffsetsTest()
  {
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

    AnalysisDataService::Instance().add("toOffsets",WS);
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
    if ( !offsets.isInitialized() ) offsets.initialize();

    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("InputWorkspace","toOffsets") );
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
    AnalysisDataService::Instance().remove("toOffsets");
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
    numpixels = 100000;
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numpixels,200);
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
  }

  void test_performance()
  {
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
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = offsets.getProperty("OutputWorkspace") );
    if (!output) return;
    TS_ASSERT_DELTA( output->dataY(0)[0], -0.0196, 0.0001);
  }

};

#endif /*GETDETECTOROFFSETSTEST_H_*/
