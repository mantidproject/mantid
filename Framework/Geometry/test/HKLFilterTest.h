// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"

#include <memory>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;

class HKLFilterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLFilterTest *createSuite() { return new HKLFilterTest(); }
  static void destroySuite(HKLFilterTest *suite) { delete suite; }

  void testFn() {
    MockHKLFilter filter;
    EXPECT_CALL(filter, isAllowed(_)).Times(2).WillOnce(Return(true)).WillRepeatedly(Return(false));

    std::function<bool(const V3D &)> f = filter.fn();

    TS_ASSERT_EQUALS(f(V3D(1, 1, 1)), true);
    TS_ASSERT_EQUALS(f(V3D(1, 1, 1)), false);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&filter));
  }

  void testUnaryLogicOperation() {
    HKLFilter_const_sptr filter = std::make_shared<MockHKLFilter>();

    TS_ASSERT_THROWS_NOTHING(MockHKLFilterUnaryLogicOperation op(filter));

    MockHKLFilterUnaryLogicOperation op(filter);
    TS_ASSERT_EQUALS(op.getOperand(), filter);

    HKLFilter_const_sptr invalid;
    TS_ASSERT_THROWS(MockHKLFilterUnaryLogicOperation op(invalid), const std::runtime_error &);
  }

  void testBinaryLogicOperation() {
    HKLFilter_const_sptr lhs = std::make_shared<MockHKLFilter>();
    HKLFilter_const_sptr rhs = std::make_shared<MockHKLFilter>();

    TS_ASSERT_THROWS_NOTHING(MockHKLFilterBinaryLogicOperation op(lhs, rhs));

    MockHKLFilterBinaryLogicOperation op(lhs, rhs);
    TS_ASSERT_EQUALS(op.getLHS(), lhs);
    TS_ASSERT_EQUALS(op.getRHS(), rhs);

    HKLFilter_const_sptr invalid;
    TS_ASSERT_THROWS(MockHKLFilterBinaryLogicOperation op(invalid, rhs), const std::runtime_error &);
    TS_ASSERT_THROWS(MockHKLFilterBinaryLogicOperation op(lhs, invalid), const std::runtime_error &);
    TS_ASSERT_THROWS(MockHKLFilterBinaryLogicOperation op(invalid, invalid), const std::runtime_error &);
  }

  void testHKLFilterNot() {
    std::shared_ptr<const MockHKLFilter> filter = std::make_shared<MockHKLFilter>();

    EXPECT_CALL(*filter, isAllowed(_)).WillOnce(Return(true)).WillOnce(Return(false));

    HKLFilterNot notFilter(filter);

    TS_ASSERT_EQUALS(notFilter.isAllowed(V3D(1, 1, 1)), false);
    TS_ASSERT_EQUALS(notFilter.isAllowed(V3D(1, 1, 1)), true);

    TS_ASSERT(Mock::VerifyAndClearExpectations(std::const_pointer_cast<MockHKLFilter>(filter).get()));
  }

  void testHKLFilterNotOperator() {
    HKLFilter_const_sptr filter = std::make_shared<MockHKLFilter>();

    HKLFilter_const_sptr notFilter = ~filter;

    std::shared_ptr<const HKLFilterNot> notFilterCasted = std::dynamic_pointer_cast<const HKLFilterNot>(notFilter);
    TS_ASSERT(notFilterCasted);

    TS_ASSERT_EQUALS(notFilterCasted->getOperand(), filter);
  }

  void testHKLFilterAnd() {
    std::shared_ptr<const MockHKLFilter> lhs = std::make_shared<MockHKLFilter>();
    EXPECT_CALL(*lhs, isAllowed(_)).WillOnce(Return(true)).WillOnce(Return(false)).WillOnce(Return(true));

    std::shared_ptr<const MockHKLFilter> rhs = std::make_shared<MockHKLFilter>();
    EXPECT_CALL(*rhs, isAllowed(_)).WillOnce(Return(true)).WillOnce(Return(false));

    HKLFilterAnd andFilter(lhs, rhs);

    TS_ASSERT_EQUALS(andFilter.isAllowed(V3D(1, 1, 1)), true);
    TS_ASSERT_EQUALS(andFilter.isAllowed(V3D(1, 1, 1)), false);
    TS_ASSERT_EQUALS(andFilter.isAllowed(V3D(1, 1, 1)), false);

    TS_ASSERT(Mock::VerifyAndClearExpectations(std::const_pointer_cast<MockHKLFilter>(lhs).get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(std::const_pointer_cast<MockHKLFilter>(rhs).get()));
  }

  void testHKLFilterAndOperator() {
    HKLFilter_const_sptr lhs = std::make_shared<MockHKLFilter>();
    HKLFilter_const_sptr rhs = std::make_shared<MockHKLFilter>();

    HKLFilter_const_sptr andFilter = lhs & rhs;

    std::shared_ptr<const HKLFilterAnd> andFilterCasted = std::dynamic_pointer_cast<const HKLFilterAnd>(andFilter);

    TS_ASSERT(andFilterCasted);

    TS_ASSERT_EQUALS(andFilterCasted->getLHS(), lhs);
    TS_ASSERT_EQUALS(andFilterCasted->getRHS(), rhs);
  }

  void testHKLFilterOr() {
    std::shared_ptr<const MockHKLFilter> lhs = std::make_shared<MockHKLFilter>();
    EXPECT_CALL(*lhs, isAllowed(_))
        .WillOnce(Return(true))
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    std::shared_ptr<const MockHKLFilter> rhs = std::make_shared<MockHKLFilter>();
    EXPECT_CALL(*rhs, isAllowed(_)).WillOnce(Return(false)).WillOnce(Return(true));

    HKLFilterOr orFilter(lhs, rhs);

    TS_ASSERT_EQUALS(orFilter.isAllowed(V3D(1, 1, 1)), true);
    TS_ASSERT_EQUALS(orFilter.isAllowed(V3D(1, 1, 1)), false);
    TS_ASSERT_EQUALS(orFilter.isAllowed(V3D(1, 1, 1)), true);
    TS_ASSERT_EQUALS(orFilter.isAllowed(V3D(1, 1, 1)), true);

    TS_ASSERT(Mock::VerifyAndClearExpectations(std::const_pointer_cast<MockHKLFilter>(lhs).get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(std::const_pointer_cast<MockHKLFilter>(rhs).get()));
  }

  void testHKLFilterOrOperator() {
    HKLFilter_const_sptr lhs = std::make_shared<MockHKLFilter>();
    HKLFilter_const_sptr rhs = std::make_shared<MockHKLFilter>();

    HKLFilter_const_sptr orFilter = lhs | rhs;

    std::shared_ptr<const HKLFilterOr> orFilterCasted = std::dynamic_pointer_cast<const HKLFilterOr>(orFilter);

    TS_ASSERT(orFilterCasted);

    TS_ASSERT_EQUALS(orFilterCasted->getLHS(), lhs);
    TS_ASSERT_EQUALS(orFilterCasted->getRHS(), rhs);
  }
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
private:
  class MockHKLFilter final : public HKLFilter {
  public:
    MOCK_CONST_METHOD0(getDescription, std::string());
    MOCK_CONST_METHOD1(isAllowed, bool(const V3D &));
  };

  class MockHKLFilterUnaryLogicOperation : public HKLFilterUnaryLogicOperation {
  public:
    MockHKLFilterUnaryLogicOperation(const HKLFilter_const_sptr &filter) : HKLFilterUnaryLogicOperation(filter) {}

    MOCK_CONST_METHOD0(getDescription, std::string());
    MOCK_CONST_METHOD1(isAllowed, bool(const V3D &));
  };

  class MockHKLFilterBinaryLogicOperation : public HKLFilterBinaryLogicOperation {
  public:
    MockHKLFilterBinaryLogicOperation(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs)
        : HKLFilterBinaryLogicOperation(lhs, rhs) {}

    MOCK_CONST_METHOD0(getDescription, std::string());
    MOCK_CONST_METHOD1(isAllowed, bool(const V3D &));
  };
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
