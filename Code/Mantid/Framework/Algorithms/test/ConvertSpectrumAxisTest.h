#ifndef CONVERTSPECTRUMAXISTEST_H_
#define CONVERTSPECTRUMAXISTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/ConvertSpectrumAxis.h"
#include "MantidDataHandling/LoadRaw3.h"

#include "MantidAPI/AnalysisDataService.h"

class ConvertSpectrumAxisTest : public CxxTest::TestSuite
{
public:
  //ConvertToDistributionTest() : dist("notDist")
  //{
  //  Workspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,10,0,0.5);
  //  AnalysisDataService::Instance().add(dist,WS);
  //}

	void testName()
	{
    TS_ASSERT_EQUALS( conv.name(), "ConvertSpectrumAxis" );
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( conv.version(), 1 );
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( conv.category(), "Units" );
	}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( conv.initialize() );
    TS_ASSERT( conv.isInitialized() );
  }

  void testExec()
  {
    using namespace Mantid::API;

    if ( !conv.isInitialized() ) conv.initialize();

    const std::string inputWS("inWS");
    const std::string outputWS("outWS");

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","2");
    loader.setPropertyValue("SpectrumMax","3");
    loader.execute();

    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Target","theta") );

    TS_ASSERT_THROWS_NOTHING( conv.execute() );
    TS_ASSERT( conv.isExecuted() );

    MatrixWorkspace_const_sptr input,output;
    TS_ASSERT_THROWS_NOTHING( input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWS)) );
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) );

    // Should now have a numeric axis up the side, with units of angle
    const Axis* thetaAxis;
    TS_ASSERT_THROWS_NOTHING( thetaAxis = output->getAxis(1) );
    TS_ASSERT( thetaAxis->isNumeric() );
    TS_ASSERT_EQUALS( thetaAxis->unit()->caption(), "Scattering angle" );
    TS_ASSERT_EQUALS( thetaAxis->unit()->label(), "degrees" );
    TS_ASSERT_DELTA( (*thetaAxis)(0), 6.1751, 0.0001 );
    TS_ASSERT_DELTA( (*thetaAxis)(1), 180.0, 0.0001 );
    // Check axis is correct length
    TS_ASSERT_THROWS( (*thetaAxis)(2), Mantid::Kernel::Exception::IndexError );

    // Data should be swapped over
    TS_ASSERT_EQUALS( input->readX(0), output->readX(1) );
    TS_ASSERT_EQUALS( input->readY(0), output->readY(1) );
    TS_ASSERT_EQUALS( input->readE(0), output->readE(1) );
    TS_ASSERT_EQUALS( input->readX(1), output->readX(0) );
    TS_ASSERT_EQUALS( input->readY(1), output->readY(0) );
    TS_ASSERT_EQUALS( input->readE(1), output->readE(0) );

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::ConvertSpectrumAxis conv;
};

#endif /*CONVERTSPECTRUMAXISTEST_H_*/
