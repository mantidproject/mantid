#ifndef NORMALISEBYCURRENTTEST_H_
#define NORMALISEBYCURRENTTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class NormaliseByCurrentTest : public CxxTest::TestSuite
{
public:
  NormaliseByCurrentTest()
  {
    AnalysisDataService::Instance().add("normIn",WorkspaceCreationHelper::Create2DWorkspace123(10,3,1));
  }

  void testName()
  {
    TS_ASSERT_EQUALS( norm.name(), "NormaliseByCurrent" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( norm.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( norm.category(), "General" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( norm.initialize() )
    TS_ASSERT( norm.isInitialized() )
  }

  void testExec()
  {
    if ( !norm.isInitialized() ) norm.initialize();

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( norm.execute(), std::runtime_error )
    TS_ASSERT( ! norm.isExecuted() )

    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("InputWorkspace","normIn") )
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("OutputWorkspace","normOut") )

    // Check it fails if charge hasn't been set.
    TS_ASSERT( ! norm.execute() )
    TS_ASSERT( ! norm.isExecuted() )
    // Now set the charge
    Workspace_sptr input = AnalysisDataService::Instance().retrieve("normIn");
    input->getSample()->setProtonCharge(2.0);

    TS_ASSERT_THROWS_NOTHING( norm.execute() )
    TS_ASSERT( norm.isExecuted() )

    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("normOut") )

    Workspace::const_iterator inIt(*input);
    for (Workspace::const_iterator it(*output); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() )
      TS_ASSERT_EQUALS( it->Y(), 1.0 )
      TS_ASSERT_EQUALS( it->E(), 1.5 )
    }
  }

private:
  NormaliseByCurrent norm;
};

#endif /*NORMALISEBYCURRENTTEST_H_*/
