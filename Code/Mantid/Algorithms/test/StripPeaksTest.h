#ifndef STRIPPEAKSTEST_H_
#define STRIPPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/StripPeaks.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidCurveFitting/Gaussian.h"

using namespace Mantid::API;
using Mantid::Algorithms::StripPeaks;

class StripPeaksTest : public CxxTest::TestSuite
{
public:
  StripPeaksTest()
  {
    Workspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(2,100,0.5,0.02);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    std::vector<double> &X = WS->dataX(1);
    std::vector<double> &Y = WS->dataY(1);
    std::vector<double> &E = WS->dataE(1);
    std::vector<double> &Y0 = WS->dataY(0);
    for (int i = 0; i < Y.size(); ++i)
    {
      const double x = (X[i]+X[i+1])/2;
      double funcVal = 2500*exp(-0.5*pow((x-2.14)/0.012,2));
      funcVal += 1000*exp(-0.5*pow((x-1.22)/0.01,2));
      Y[i] = 5000 + funcVal;
      E[i] = sqrt(Y[i]);

      Y0[i] = 5000;
    }

    AnalysisDataService::Instance().add("toStrip",WS);
  }

	void testName()
	{
    TS_ASSERT_EQUALS( strip.name(), "StripPeaks" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( strip.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( strip.category(), "General" )
	}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( strip.initialize() )
    TS_ASSERT( strip.isInitialized() )
  }

  void testExec()
  {
    if ( !strip.isInitialized() ) strip.initialize();

    TS_ASSERT_THROWS_NOTHING( strip.setPropertyValue("InputWorkspace","toStrip") )
    std::string outputWS("stripped");
    TS_ASSERT_THROWS_NOTHING( strip.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( strip.execute() )
    TS_ASSERT( strip.isExecuted() )

    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve(outputWS) )

    Workspace_const_sptr input = AnalysisDataService::Instance().retrieve("toStrip");
    Workspace::const_iterator inIt(*input);
    for (Workspace::const_iterator it(*output); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() )
      TS_ASSERT_DELTA( it->Y(), 5000.0, 0.5 )
      TS_ASSERT_EQUALS( it->E(), inIt->E() )
    }

    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  StripPeaks strip;
};

#endif /*STRIPPEAKSTEST_H_*/
