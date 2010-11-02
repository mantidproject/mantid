#ifndef GETDETECTOROFFSETSTEST_H_
#define GETDETECTOROFFSETSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidCurveFitting/GaussianLinearBG1D.h"

using namespace Mantid::API;
using Mantid::Algorithms::GetDetectorOffsets;

class GetDetectorOffsetsTest : public CxxTest::TestSuite
{
public:
  GetDetectorOffsetsTest()
  {
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,200,-100.5,1);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    const Mantid::MantidVec &X = WS->readX(0);
    Mantid::MantidVec &Y = WS->dataY(0);
    Mantid::MantidVec &E = WS->dataE(0);
    for (int i = 0; i < Y.size(); ++i)
    {
      const double x = (X[i]+X[i+1])/2;
      Y[i] = exp(-0.5*pow((x-1),2));
      E[i] = sqrt(Y[i]);
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
    std::string outputFile;
    outputFile = "GetDetOffsets.cal";
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("GroupingFileName", outputFile));

    TS_ASSERT_THROWS_NOTHING( offsets.execute() );
    TS_ASSERT( offsets.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) );

    TS_ASSERT_DELTA( output->dataY(0)[0], -0.0099, 0.0001);


    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove("toOffsets");
    //Remove file; empty since detectors not set
    Poco::File(outputFile).remove();

  }

private:
  GetDetectorOffsets offsets;
};

#endif /*GETDETECTOROFFSETSTEST_H_*/
