#ifndef CONVERTTODISTRIBUTIONTEST_H_
#define CONVERTTODISTRIBUTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::Algorithms::ConvertToDistribution;

class ConvertToDistributionTest : public CxxTest::TestSuite
{
public:
  static ConvertToDistributionTest *createSuite() { return new ConvertToDistributionTest(); }
  static void destroySuite(ConvertToDistributionTest *suite) { delete suite; }

  ConvertToDistributionTest() : dist("notDist")
  {
    Workspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,10,0,0.5);
    AnalysisDataService::Instance().add(dist,WS);
  }

	void testName()
	{
    TS_ASSERT_EQUALS( conv.name(), "ConvertToDistribution" )
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
      TS_ASSERT_EQUALS( Y[i], 4 )
      TS_ASSERT_EQUALS( E[i], sqrt(2.0)/0.5 )
    }
    TS_ASSERT( output->isDistribution() )

    AnalysisDataService::Instance().remove(dist);
  }

private:
  ConvertToDistribution conv;
  std::string dist;
};

#endif /*CONVERTTODISTRIBUTIONTEST_H_*/
