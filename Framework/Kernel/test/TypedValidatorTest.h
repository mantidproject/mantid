// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DataItem.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <memory>

#define DECLARE_TEST_VALIDATOR(ClassName, HeldType)                                                                    \
  class ClassName : public Mantid::Kernel::TypedValidator<HeldType> {                                                  \
  public:                                                                                                              \
    Mantid::Kernel::IValidator_sptr clone() const override { return std::make_shared<ClassName>(); }                   \
    std::string checkValidity(const HeldType &) const override { return ""; }                                          \
  };

/// Dummy object to hold in a shared_ptr for test
struct Holder {};
DECLARE_TEST_VALIDATOR(SharedPtrTypedValidator, std::shared_ptr<Holder>)
DECLARE_TEST_VALIDATOR(PODTypedValidator, double)
class FakeDataItem : public Mantid::Kernel::DataItem {
public:
  const std::string id() const override { return "FakeDataItem"; }
  const std::string &getName() const override { return m_name; }
  bool threadSafe() const override { return true; }
  const std::string toString() const override { return "FakeDataItem{}"; }

private:
  std::string m_name{"Empty"};
};
DECLARE_TEST_VALIDATOR(DataItemSptrTypedValidator, std::shared_ptr<FakeDataItem>)

class TypedValidatorTest : public CxxTest::TestSuite {
public:
  void test_shared_ptr_is_passed_successfully_to_concrete_validator() {
    Mantid::Kernel::IValidator_sptr valueChecker = std::make_shared<SharedPtrTypedValidator>();
    const std::shared_ptr<Holder> testPtr = std::make_shared<Holder>();

    checkIsValidReturnsEmptyString<std::shared_ptr<Holder>>(valueChecker, testPtr);
  }

  void test_simple_type_passed_successfully_to_concrete_validator() {
    Mantid::Kernel::IValidator_sptr valueChecker = std::make_shared<PODTypedValidator>();

    checkIsValidReturnsEmptyString<double>(valueChecker, 10.0);
  }

  void test_DataItem_sptr_descendent_is_passed_successfully_to_concrete_validator() {
    Mantid::Kernel::IValidator_sptr valueChecker = std::make_shared<DataItemSptrTypedValidator>();
    std::shared_ptr<FakeDataItem> fakeData = std::make_shared<FakeDataItem>();
    checkIsValidReturnsEmptyString<std::shared_ptr<FakeDataItem>>(valueChecker, fakeData);
  }

private:
  template <typename HeldType>
  void checkIsValidReturnsEmptyString(const Mantid::Kernel::IValidator_sptr &valueChecker, const HeldType &value) {
    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = valueChecker->isValid(value));
    TS_ASSERT_EQUALS(error, "");
  }
};
