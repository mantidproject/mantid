#ifndef MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMDTEST_H_
#define MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidMDAlgorithms/BooleanBinaryOperationMD.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace testing;

class MockBooleanBinaryOperationMD : public BooleanBinaryOperationMD
{
public:
  MOCK_METHOD0(initExtraProperties, void());
  MOCK_METHOD2(execHistoHisto, void(Mantid::MDEvents::MDHistoWorkspace_sptr, Mantid::MDEvents::MDHistoWorkspace_const_sptr));
};


class BooleanBinaryOperationMDTest : public CxxTest::TestSuite
{
public:

  void test_basics()
  {
    MockBooleanBinaryOperationMD alg;
    EXPECT_CALL(alg, initExtraProperties()).Times(1);
    alg.initialize();
    TSM_ASSERT("Algorithm methods were called as expected", testing::Mock::VerifyAndClearExpectations(&alg));
  }


};


#endif /* MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMDTEST_H_ */
