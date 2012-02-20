#ifndef CONJOINWORKSPACESTEST_H_
#define CONJOINWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

/** NOTE! The real tests are in ConjoinWorkspace2Test */
class ConjoinWorkspacesTest : public CxxTest::TestSuite
{
public:

  /** InputWorkspace1 is set as the output workspace */
  void test_InputWorkspace1_is_output()
  {
    ConjoinWorkspaces conj;
    if ( !conj.isInitialized() ) conj.initialize();

    MatrixWorkspace_sptr in1 = WorkspaceCreationHelper::Create2DWorkspace(5, 10);
    MatrixWorkspace_sptr in2 = WorkspaceCreationHelper::Create2DWorkspace(7, 10);
    AnalysisDataService::Instance().addOrReplace("top", in1);
    AnalysisDataService::Instance().addOrReplace("bottom", in2);

    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace1","top") );
    TS_ASSERT_THROWS_NOTHING( conj.setPropertyValue("InputWorkspace2","bottom") );
    TS_ASSERT_THROWS_NOTHING( conj.setProperty("CheckOverlapping", false) );
    TS_ASSERT_THROWS_NOTHING( conj.execute() );
    TS_ASSERT( conj.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("top")) );
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 12 );

    // Check that 2nd input workspace no longer exists
    TS_ASSERT_THROWS( AnalysisDataService::Instance().retrieve("bottom"), Exception::NotFoundError );
  }

};

#endif /*CONJOINWORKSPACESTEST_H_*/
