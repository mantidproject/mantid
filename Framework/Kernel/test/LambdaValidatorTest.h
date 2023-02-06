// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/IValidator.h"
#include "MantidKernel/LambdaValidator.h"

using Mantid::Kernel::IValidator_sptr;
using Mantid::Kernel::LambdaValidator;

class LambdaValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LambdaValidatorTest *createSuite() { return new LambdaValidatorTest(); }
  static void destroySuite(LambdaValidatorTest *suite) { delete suite; }

  void testEmptyLambda() {
    LambdaValidator<int> validator([](int x) {
      // trivial if to remove unused warning
      if (x == 0) {
      }
      return "";
    });
    TS_ASSERT_EQUALS("", validator.isValid(2));
  }

  void testSampleValidatorLambda() {
    const int validInput = 4;
    const int invalidInput = 5;
    const std::string error = "Error";

    LambdaValidator<int> validator([error](int x) { return (x % 2) == 0 ? "" : error; });

    TS_ASSERT_EQUALS("", validator.isValid(validInput));
    TS_ASSERT_EQUALS(error, validator.isValid(invalidInput));
  }

  void testChangeFunction() {
    const std::string error = "Error";

    LambdaValidator<int> validator([error](int x) { return (x % 2) == 0 ? "" : error; });

    int input = 4;
    TS_ASSERT_EQUALS("", validator.isValid(input));

    validator.setValidatorFunction([error](int x) { return (x % 2) != 0 ? "" : error; });

    input = 5;
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  void testClone() {
    IValidator_sptr v(new LambdaValidator<int>([](int x) {
      // trivial if to remove unused warning
      if (x == 0) {
      }
      return "";
    }));
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS(v, vv);
    TS_ASSERT(std::dynamic_pointer_cast<LambdaValidator<int>>(vv));
  }
};
