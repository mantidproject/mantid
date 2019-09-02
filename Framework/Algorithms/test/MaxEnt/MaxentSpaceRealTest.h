// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTSPACEREALTEST_H_
#define MANTID_ALGORITHMS_MAXENTSPACEREALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"

using Mantid::Algorithms::MaxentSpaceReal;

class MaxentSpaceRealTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentSpaceRealTest *createSuite() {
    return new MaxentSpaceRealTest();
  }
  static void destroySuite(MaxentSpaceRealTest *suite) { delete suite; }

  void test_toComplex() {
    // A space of real numbers
    MaxentSpaceReal space;
    // A vector of complex numbers
    std::vector<double> values = {-1., 0., 1., 2.};

    auto toComplex = space.toComplex(values);
    // Size must be 2*N
    TS_ASSERT_EQUALS(toComplex.size(), 8);
    // Real components
    TS_ASSERT_EQUALS(toComplex[0], -1.);
    TS_ASSERT_EQUALS(toComplex[2], 0.);
    TS_ASSERT_EQUALS(toComplex[4], 1.);
    TS_ASSERT_EQUALS(toComplex[6], 2.);
    // Complex components (all must be zero)
    TS_ASSERT_EQUALS(toComplex[1], 0.);
    TS_ASSERT_EQUALS(toComplex[3], 0.);
    TS_ASSERT_EQUALS(toComplex[5], 0.);
    TS_ASSERT_EQUALS(toComplex[7], 0.);
  }

  void test_fromComplex() {
    // A space of real numbers
    MaxentSpaceReal space;
    // A vector of complex numbers
    std::vector<double> values = {-1., 0., 1., 2., 3., 4.};

    auto fromComplex = space.fromComplex(values);
    // Size must be N/2
    TS_ASSERT_EQUALS(fromComplex.size(), 3);
    // Real components
    TS_ASSERT_EQUALS(fromComplex[0], -1.);
    TS_ASSERT_EQUALS(fromComplex[1], 1.);
    TS_ASSERT_EQUALS(fromComplex[2], 3.);
  }

  void test_fromComplex_bad_size() {
    // A space of real numbers
    MaxentSpaceReal space;
    // A vector of complex numbers
    std::vector<double> values = {-1., 0., 1., 2., 3.};

    // Odd number of elements, we can't convert to real vector
    TS_ASSERT_THROWS(space.fromComplex(values), const std::invalid_argument &);
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTSPACEREALTEST_H_ */