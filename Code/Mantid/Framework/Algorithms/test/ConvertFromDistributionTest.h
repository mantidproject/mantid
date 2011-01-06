#ifndef CONVERTFROMDISTRIBUTIONTEST_H_
#define CONVERTFROMDISTRIBUTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertFromDistribution.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::Algorithms::ConvertFromDistribution;

class ConvertFromDistributionTest : public CxxTest::TestSuite
{
public:
  ConvertFromDistributionTest() : dist("dist")
  {
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,10,0,0.5);
    WS->isDistribution(true);
    AnalysisDataService::Instance().add(dist,WS);
  }

	void testName()
	{
    TS_ASSERT_EQUALS( conv.name(), "ConvertFromDistribution" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( conv.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( conv.category(), "General" )
	}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( conv.initialize() )
    TS_ASSERT( conv.isInitialized() )
  }

  void testExec()
  {
    if ( !conv.isInitialized() ) conv.initialize();

    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("Workspace",dist) )

    TS_ASSERT_THROWS_NOTHING( conv.execute() )
    TS_ASSERT( conv.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(dist)) )

    const Mantid::MantidVec &X = output->dataX(0);
    const Mantid::MantidVec &Y = output->dataY(0);
    const Mantid::MantidVec &E = output->dataE(0);
    for (int i = 0; i < Y.size(); ++i)
    {
      TS_ASSERT_EQUALS( X[i], i/2.0 )
      TS_ASSERT_EQUALS( Y[i], 1 )
      TS_ASSERT_EQUALS( E[i], sqrt(2.0)/2.0 )
    }
    TS_ASSERT( ! output->isDistribution() )

    AnalysisDataService::Instance().remove(dist);
  }

private:
  ConvertFromDistribution conv;
  std::string dist;
};

#endif /*CONVERTFROMDISTRIBUTIONTEST_H_*/
