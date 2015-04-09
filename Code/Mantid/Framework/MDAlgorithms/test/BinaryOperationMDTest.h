#ifndef MANTID_MDALGORITHMS_BINARYOPERATIONMDTEST_H_
#define MANTID_MDALGORITHMS_BINARYOPERATIONMDTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace testing;

class MockBinaryOperationMD : public BinaryOperationMD
{
public:
  MOCK_CONST_METHOD0(commutative, bool());
  MOCK_METHOD0(checkInputs, void());
  MOCK_METHOD0(execEvent, void());
  MOCK_METHOD2(execHistoHisto, void(Mantid::DataObjects::MDHistoWorkspace_sptr, Mantid::DataObjects::MDHistoWorkspace_const_sptr));
  MOCK_METHOD2(execHistoScalar, void(Mantid::DataObjects::MDHistoWorkspace_sptr, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar));
  void exec()
  { BinaryOperationMD::exec();  }
};


class BinaryOperationMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinaryOperationMDTest *createSuite() { return new BinaryOperationMDTest(); }
  static void destroySuite( BinaryOperationMDTest *suite ) { delete suite; }

  MDHistoWorkspace_sptr histo_A;
  MDHistoWorkspace_sptr histo_B;
  MDHistoWorkspace_sptr histo2d_100;
  MDHistoWorkspace_sptr histo3d;
  IMDEventWorkspace_sptr event_A;
  IMDEventWorkspace_sptr event_B;
  WorkspaceSingleValue_sptr scalar;
  IMDWorkspace_sptr out;

  void setUp()
  {
    histo_A = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 5, 10.0, 1.0);
    histo_B = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 5, 10.0, 1.0);
    histo2d_100 = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10, 10.0, 1.0);
    histo3d = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 5, 10.0, 1.0);
    event_A = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    event_B = MDEventsTestHelper::makeMDEW<2>(3, 0.0, 10.0, 1);
    scalar = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.5);
    AnalysisDataService::Instance().addOrReplace("histo_A", histo_A);
    AnalysisDataService::Instance().addOrReplace("histo_B", histo_B);
    AnalysisDataService::Instance().addOrReplace("histo2d_100", histo2d_100);
    AnalysisDataService::Instance().addOrReplace("histo3d", histo3d);
    AnalysisDataService::Instance().addOrReplace("event_A", event_A);
    AnalysisDataService::Instance().addOrReplace("event_B", event_B);
    AnalysisDataService::Instance().addOrReplace("scalar", scalar);
  }

  void test_Init()
  {
    MockBinaryOperationMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  /// Sub-class can abort
  void test_checkInputs()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, checkInputs()).WillOnce(Throw( std::runtime_error("Bad inputs!") ));
    doTest(alg, "histo_A", "histo_B", "some_output", false /*it fails*/ );
  }

  /// Run the mock algorithm
  void doTest(MockBinaryOperationMD & alg, std::string lhs, std::string rhs, std::string outName,
      bool succeeds=true)
  {
    out.reset();
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("LHSWorkspace", lhs );
    alg.setPropertyValue("RHSWorkspace", rhs );
    alg.setPropertyValue("OutputWorkspace", outName );
    alg.execute();
    if (succeeds)
    {
      TS_ASSERT( alg.isExecuted() );
      TSM_ASSERT("Algorithm methods were called as expected", testing::Mock::VerifyAndClearExpectations(&alg));
      out = boost::dynamic_pointer_cast<IMDWorkspace>( AnalysisDataService::Instance().retrieve(outName));
      TS_ASSERT( out );
      auto outHisto = boost::dynamic_pointer_cast<MDHistoWorkspace>(out);
      if (outHisto)
      {
        TS_ASSERT( outHisto->getExperimentInfo(0)->run().hasProperty("mdhisto_was_modified") );
      }
    }
    else
    {
      TS_ASSERT( !alg.isExecuted() );
      TSM_ASSERT("Algorithm methods were called as expected", testing::Mock::VerifyAndClearExpectations(&alg));
    }
  }


  /// 3D + 2D = NOT ALLOWED
  void test_mimatched_dimensions_fails()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    doTest(alg, "histo_A", "histo3d", "new_out", false);
  }

  /// 100 points + 25 points = NOT ALLOWED
  void test_mimatched_size_fails()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    doTest(alg, "histo_A", "histo2d_100", "new_out", false);
  }

  /// A = 2 + 3  = NOT ALLOWED!
  void test_scalar_scalar_fails()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).Times(0);
    doTest(alg, "scalar", "scalar", "some_output", false /*it fails*/ );
  }


  //==========================================================================================
  //=============================== Histo * Histo cases ======================================
  //==========================================================================================

  /// C = A + B
  void test_histo()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoHisto(_,_)).WillOnce(Return());

    doTest(alg, "histo_A", "histo_B", "new_out");

    TS_ASSERT( out != histo_A);
    TS_ASSERT( out != histo_B);
    TS_ASSERT_EQUALS( out->getNPoints(), histo_B->getNPoints());
  }
  
  /// A = A + B -> A += B
  void test_histo_inplace()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoHisto(_,_)).WillOnce(Return());

    doTest(alg, "histo_A", "histo_B", "histo_A");

    TS_ASSERT( out == histo_A);
    TS_ASSERT( out != histo_B);
  }

  /// A = B * A  ->  A *= B
  void test_histo_inplace_commutative()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoHisto(_,_)).WillOnce(Return());

    doTest(alg, "histo_B", "histo_A", "histo_A");

    TS_ASSERT( out == histo_A);
    TS_ASSERT( out != histo_B);
  }

  /// A = B / A  ->  B /= A
  void test_histo_inplace_not_commutative()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(false));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoHisto(_,_)).WillOnce(Return());

    doTest(alg, "histo_B", "histo_A", "histo_A");

    TS_ASSERT( out != histo_A); // Output is neither A nor B
    TS_ASSERT( out != histo_B);
    TS_ASSERT_EQUALS( out->getNPoints(), histo_B->getNPoints());
  }

  //==========================================================================================
  //=============================== Histo * Scalar cases =====================================
  //==========================================================================================


  /// B = A * 2
  void test_histo_scalar()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, execHistoScalar(_,_)).WillOnce(Return());
    doTest(alg, "histo_A", "scalar", "some_output");
    TS_ASSERT( out != histo_A); // Output is new
    TS_ASSERT_EQUALS( out->getNPoints(), histo_A->getNPoints()); // Output is clone
  }

  /// A = A * 2
  void test_histo_scalar_inplace()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoScalar(_,_)).WillOnce(Return());
    doTest(alg, "histo_A", "scalar", "histo_A");
    TSM_ASSERT("Done in-place", out == histo_A);
  }

  /// B = 2 * A
  void test_scalar_histo_commutative()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoScalar(_,_)).WillOnce(Return());
    doTest(alg, "scalar", "histo_A", "some_output");
    TS_ASSERT( out != histo_A); // Output is new
    TS_ASSERT_EQUALS( out->getNPoints(), histo_A->getNPoints()); // Output is clone
  }

  /// B = 2 / A = NOT ALLOWED
  void test_scalar_histo_non_commutative_fails()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(false));
    doTest(alg, "scalar", "histo_A", "some_output", false /*it fails*/ );
  }

  /// A = 2 * A
  void test_scalar_histo_inplace_commutative()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execHistoScalar(_,_)).WillOnce(Return());
    doTest(alg, "scalar", "histo_A", "histo_A");
    TSM_ASSERT("Done in-place", out == histo_A);
  }

  //==========================================================================================
  //=============================== MDEventWorkspace cases ===================================
  //==========================================================================================

  /// C = A * B
  void test_event_event()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent()).WillOnce(Return());
    doTest(alg, "event_A", "event_B", "other_output");
    TSM_ASSERT("Not Done in-place", out != histo_A);
  }

  /// A = A * B -> A *= B
  void test_event_event_inplace()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent()).WillOnce(Return());
    doTest(alg, "event_A", "event_B", "event_A");
    TSM_ASSERT("Done in-place", out == event_A);
  }

  /// A = B * A -> A *= B
  void test_event_event_inplace_commuting()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent()).WillOnce(Return());
    doTest(alg, "event_B", "event_A", "event_A");
    TSM_ASSERT("Done in-place", out == event_A);
  }

  /// A = B / A
  void test_event_event_inplace_non_commuting()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(false));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent()).WillOnce(Return());
    doTest(alg, "event_B", "event_A", "event_A");
    TSM_ASSERT("Output replaced A", out != event_A);
    TS_ASSERT_EQUALS( out->getNPoints(), event_A->getNPoints()); // Output is clone
  }

  /// C = A * 2
  void test_event_scalar()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent()).WillOnce(Return());
    doTest(alg, "event_A", "scalar", "other_output");
    TSM_ASSERT("Not Done in-place", out != histo_A);
  }

  /// C = A * histo -> Will pass through
  void test_event_histo()
  {
    MockBinaryOperationMD alg;
    EXPECT_CALL(alg, commutative()).WillRepeatedly(Return(true));
    EXPECT_CALL(alg, checkInputs()).WillOnce(DoDefault());
    EXPECT_CALL(alg, execEvent()).WillOnce(Return());
    doTest(alg, "event_A", "histo_A", "other_output");
  }

};


#endif /* MANTID_MDALGORITHMS_BINARYOPERATIONMDTEST_H_ */
