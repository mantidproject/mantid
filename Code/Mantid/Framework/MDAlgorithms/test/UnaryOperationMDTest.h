#ifndef MANTID_MDALGORITHMS_UNARYOPERATIONMDTEST_H_
#define MANTID_MDALGORITHMS_UNARYOPERATIONMDTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidMDAlgorithms/UnaryOperationMD.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace testing;

class MockUnaryOperationMD : public UnaryOperationMD
{
public:
  MOCK_METHOD1(execEvent, void(Mantid::API::IMDEventWorkspace_sptr));
  MOCK_METHOD0(checkInputs, void());
  MOCK_METHOD1(execHisto, void(Mantid::DataObjects::MDHistoWorkspace_sptr));
  void exec()
  { UnaryOperationMD::exec();  }
};

class UnaryOperationMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnaryOperationMDTest *createSuite() { return new UnaryOperationMDTest(); }
  static void destroySuite( UnaryOperationMDTest *suite ) { delete suite; }


  MDHistoWorkspace_sptr histo;
  IMDEventWorkspace_sptr event;
  WorkspaceSingleValue_sptr scalar;
  IMDWorkspace_sptr out;

  void setUp()
  {
    histo = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 5, 10.0, 1.0);
    event = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    scalar = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.5);
    AnalysisDataService::Instance().addOrReplace("histo", histo);
    AnalysisDataService::Instance().addOrReplace("event", event);
    AnalysisDataService::Instance().addOrReplace("scalar", scalar);
  }

  /// Run the mock algorithm
  void doTest(MockUnaryOperationMD & alg, std::string inName, std::string outName,
      bool succeeds=true)
  {
    out.reset();
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("InputWorkspace", inName );
    alg.setPropertyValue("OutputWorkspace", outName );
    alg.execute();
    if (succeeds)
    {
      TS_ASSERT( alg.isExecuted() );
      TSM_ASSERT("Algorithm methods were called as expected", testing::Mock::VerifyAndClearExpectations(&alg));
      out = boost::dynamic_pointer_cast<IMDWorkspace>( AnalysisDataService::Instance().retrieve(outName));
      TS_ASSERT( out );
    }
    else
    {
      TS_ASSERT( !alg.isExecuted() );
      TSM_ASSERT("Algorithm methods were called as expected", testing::Mock::VerifyAndClearExpectations(&alg));
    }
  }

  void test_Init()
  {
    MockUnaryOperationMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  /// Sub-class can abort
  void test_checkInputs()
  {
    MockUnaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).WillOnce(Throw( std::runtime_error("Bad inputs!") ));
    doTest(alg, "histo", "some_output", false /*it fails*/ );
  }

  /// A = log(2)  = NOT ALLOWED!
  void test_scalar_fails()
  {
    MockUnaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).Times(0);
    doTest(alg, "scalar", "some_output", false /*it fails*/ );
  }

  /// B = log(A)
  void test_histo()
  {
    MockUnaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHisto(_)).WillOnce(Return());

    doTest(alg, "histo", "new_out");

    TSM_ASSERT( "Operation not performed in place.", out != histo);
    TS_ASSERT_EQUALS( out->getNPoints(), histo->getNPoints());
  }

  /// A = log(A)
  void test_histo_inPlace()
  {
    MockUnaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHisto(_)).WillOnce(Return());

    doTest(alg, "histo", "histo");

    TSM_ASSERT( "Operation performed in place.", out == histo);
  }

  /// B = log(A)
  void test_event()
  {
    MockUnaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent(_)).WillOnce(Return());

    doTest(alg, "event", "new_out");

    TSM_ASSERT( "Operation not performed in place.", out != event);
    TS_ASSERT_EQUALS( out->getNPoints(), event->getNPoints());
  }

  /// A = log(A)
  void test_event_inPlace()
  {
    MockUnaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent(_)).WillOnce(Return());

    doTest(alg, "event", "event");

    TSM_ASSERT( "Operation performed in place.", out == event);
  }

};


#endif /* MANTID_MDALGORITHMS_UNARYOPERATIONMDTEST_H_ */
