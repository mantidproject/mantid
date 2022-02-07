// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <cxxtest/TestSuite.h>
#include <string>
#include <vector>

using namespace Mantid::Kernel;
using namespace std;

class ArrayOrderedPairsValidatorTest : public CxxTest::TestSuite {
public:
  void test_double_clone() {
    IValidator_sptr vd(new ArrayOrderedPairsValidator<double>());
    IValidator_sptr vvd = vd->clone();
    TS_ASSERT_DIFFERS(vd, vvd);
  }

  void test_int_clone() {
    IValidator_sptr vi(new ArrayOrderedPairsValidator<int>);
    IValidator_sptr vvi = vi->clone();
    TS_ASSERT_DIFFERS(vi, vvi);
  }

  void test_array_validation() {
    std::vector<int> vec{1, 5, 2, 3, 10, 10};
    ArrayOrderedPairsValidator<int> validator;
    TS_ASSERT_EQUALS(validator.isValid(vec), "");
  }

  void test_array_validation_unordered() {
    std::vector<int> vec{10, 5, 3, 2, 10, 10};
    ArrayOrderedPairsValidator<int> validator;
    TS_ASSERT_EQUALS(validator.isValid(vec), "Pair (10, 5) is not ordered.\nPair (3, 2) is not ordered.\n");
  }

  void test_array_validation_odd() {
    std::vector<int> vec{1, 5, 2, 3, 10};
    ArrayOrderedPairsValidator<int> validator;
    TS_ASSERT_EQUALS(validator.isValid(vec), "Array has an odd number of entries (5).");
  }
};
