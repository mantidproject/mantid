// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTIONTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryBackgroundSubtraction.h"

using Mantid::Algorithms::ReflectometryBackgroundSubtraction;

class ReflectometryBackgroundSubtractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryBackgroundSubtractionTest *createSuite() { return new ReflectometryBackgroundSubtractionTest(); }
  static void destroySuite( ReflectometryBackgroundSubtractionTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTIONTEST_H_ */