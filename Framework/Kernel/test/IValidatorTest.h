// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/IValidator.h"
#include <cxxtest/TestSuite.h>
#include <memory>

namespace {
/**
 * Implement the validator interface to check that
 * it does not copy any data when the check method is called
 *
 * This is intimately tied to the test implementation
 */
class DataNotCopiedValidator : public Mantid::Kernel::IValidator {
public:
  DataNotCopiedValidator() : Mantid::Kernel::IValidator(), m_head(nullptr) {}

  Mantid::Kernel::IValidator_sptr clone() const override { return std::make_shared<DataNotCopiedValidator>(); }

  /// Return the stored head pointer
  const double *head() const { return m_head; }

private:
  std::string check(const boost::any &value) const override {
    using HeldType = std::vector<double>;
    const HeldType *dataPtr = boost::any_cast<const HeldType *>(value);
    m_head = dataPtr->data();
    return "";
  }

  /// The pointer to the head of the vector
  mutable const double *m_head;
};
} // namespace

class IValidatorTest : public CxxTest::TestSuite {
public:
  void test_Values_Are_Not_Copied_When_Passed_To_Concrete_Validators() {
    const std::vector<double> testData(10, 1.0);
    DataNotCopiedValidator noCopy;

    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = noCopy.isValid(testData));
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT_EQUALS(noCopy.head(), testData.data());
  }
};
