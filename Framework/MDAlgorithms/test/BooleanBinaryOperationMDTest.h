#ifndef MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMDTEST_H_
#define MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMDTEST_H_

#include "MantidKernel/WarningSuppressions.h"
#include "MantidMDAlgorithms/BooleanBinaryOperationMD.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::MDAlgorithms;
using namespace testing;
GNU_DIAG_OFF_SUGGEST_OVERRIDE
class MockBooleanBinaryOperationMD : public BooleanBinaryOperationMD {
public:
  MOCK_METHOD0(initExtraProperties, void());
  MOCK_METHOD2(execHistoHisto,
               void(Mantid::DataObjects::MDHistoWorkspace_sptr,
                    Mantid::DataObjects::MDHistoWorkspace_const_sptr));
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
class BooleanBinaryOperationMDTest : public CxxTest::TestSuite {
public:
  void test_basics() {
    MockBooleanBinaryOperationMD alg;
    EXPECT_CALL(alg, initExtraProperties()).Times(1);
    alg.initialize();
    TSM_ASSERT("Algorithm methods were called as expected",
               testing::Mock::VerifyAndClearExpectations(&alg));
  }
};

#endif /* MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMDTEST_H_ */
