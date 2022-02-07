// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"

using Mantid::Algorithms::MaxentSpaceComplex;

class MaxentSpaceComplexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentSpaceComplexTest *createSuite() { return new MaxentSpaceComplexTest(); }
  static void destroySuite(MaxentSpaceComplexTest *suite) { delete suite; }

  void test_complex_space() {

    // Space of complex numbers
    MaxentSpaceComplex space;
    // A vector of complex numbers
    std::vector<double> values = {-1., 0., 1., 2., 3., 4., 5., 6., 7., 8.};

    auto toComplex = space.toComplex(values);
    TS_ASSERT_EQUALS(toComplex, values);

    auto fromComplex = space.fromComplex(values);
    TS_ASSERT_EQUALS(fromComplex, values);
  }
};
