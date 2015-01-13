#ifndef GETDETECTOROFFSETSTEST_H_
#define GETDETECTOROFFSETSTEST_H_

#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;
using Mantid::Algorithms::GetDetectorOffsets;
using Mantid::DataObjects::OffsetsWorkspace_sptr;

class GetDetectorOffsetsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetDetectorOffsetsTest *createSuite() { return new GetDetectorOffsetsTest(); }
  static void destroySuite( GetDetectorOffsetsTest *suite ) { delete suite; }

  GetDetectorOffsetsTest()
  {
    Mantid::API::FrameworkManager::Instance();
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
    for (size_t i = 0; i < Y.size(); ++i)
    {
      const double x = (X[i]+X[i+1])/2;
      Y[i] = exp(-0.5*pow((x-1)/10.0,2));
      E[i] = 0.001;
    }

    // ---- Run algo -----
    if ( !offsets.isInitialized() ) offsets.initialize();
    TS_ASSERT_THROWS_NOTHING( offsets.setProperty("InputWorkspace",WS) );
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("MaskWorkspace",maskWS) );
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step","0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference","1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin","-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax","20"));
    TS_ASSERT_THROWS_NOTHING( offsets.execute() );
    TS_ASSERT( offsets.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );
    if (!output) return;

    TS_ASSERT_DELTA( output->dataY(0)[0], -0.0196, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING( mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(maskWS) );
    if (!mask) return;
    TS_ASSERT( !mask->getInstrument()->getDetector(1)->isMasked() );
  }


  void testExecWithGroup()
  {
    // --------- Workspace with summed spectra -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::CreateGroupedWorkspace2D(3, 200, 1.0);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    const Mantid::MantidVec &X = WS->readX(0);
    Mantid::MantidVec &Y = WS->dataY(0);
    Mantid::MantidVec &E = WS->dataE(0);
    for (size_t i = 0; i < Y.size(); ++i)
    {
      const double x = (X[i]+X[i+1])/2;
      Y[i] = exp(-0.5*pow((x-1)/10.0,2));
      E[i] = 0.001;
    }

    // ---- Run algo -----
    if ( !offsets.isInitialized() ) offsets.initialize();
    TS_ASSERT_THROWS_NOTHING( offsets.setProperty("InputWorkspace",WS) );
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("MaskWorkspace",maskWS) );
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step","0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference","1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin","-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax","20"));
    TS_ASSERT_THROWS_NOTHING( offsets.execute() );
    TS_ASSERT( offsets.isExecuted() );

    OffsetsWorkspace_sptr output = offsets.getProperty("OutputWorkspace");
    if (!output) return;

    TS_ASSERT_DELTA( output->getValue(1), -0.0196, 0.0001);
    TS_ASSERT_EQUALS( output->getValue(1), output->getValue(2));
    TS_ASSERT_EQUALS( output->getValue(1), output->getValue(3));

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING( mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(maskWS) );
    if (!mask) return;
    TS_ASSERT( !mask->getInstrument()->getDetector(1)->isMasked() );

  }

  
  void testExecAbsolute()
  {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1,200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    const Mantid::MantidVec &X = WS->readX(0);
    Mantid::MantidVec &Y = WS->dataY(0);
    Mantid::MantidVec &E = WS->dataE(0);
    for (size_t i = 0; i < Y.size(); ++i)
    {
      const double x = (X[i]+X[i+1])/2;
      Y[i] = exp(-0.5*pow((x-1)/10.0,2));
      E[i] = 0.001;
    }

    // ---- Run algo -----
    if ( !offsets.isInitialized() ) offsets.initialize();
    TS_ASSERT_THROWS_NOTHING( offsets.setProperty("InputWorkspace",WS) );
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( offsets.setPropertyValue("MaskWorkspace",maskWS) );
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step","0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference","1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin","-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax","20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset","10"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OffsetMode","Absolute"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DIdeal","3.5"));
    TS_ASSERT_THROWS_NOTHING( offsets.execute() );
    TS_ASSERT( offsets.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) );
    if (!output) return;

    TS_ASSERT_DELTA( output->dataY(0)[0], 2.4803, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING( mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(maskWS) );
    if (!mask) return;
    TS_ASSERT( !mask->getInstrument()->getDetector(1)->isMasked() );
  }


private:
  GetDetectorOffsets offsets;
};

class GetDetectorOffsetsTestPerformance : public CxxTest::TestSuite
{
  MatrixWorkspace_sptr WS;
  int numpixels;

public:
  static GetDetectorOffsetsTestPerformance *createSuite() { return new GetDetectorOffsetsTestPerformance(); }
  static void destroySuite( GetDetectorOffsetsTestPerformance *suite ) { delete suite; }

  GetDetectorOffsetsTestPerformance()
  {
    FrameworkManager::Instance();
  }

  void setUp()
  {
    numpixels = 10000;
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numpixels,200, false);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    for (size_t wi=0; wi<WS->getNumberHistograms(); wi++)
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
