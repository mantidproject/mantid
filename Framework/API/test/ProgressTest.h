// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROGRESSTEST_H_
#define PROGRESSTEST_H_

#include "MantidAPI/Progress.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class ProgressTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProgressTest *createSuite() { return new ProgressTest(); }
  static void destroySuite(ProgressTest *suite) { delete suite; }

  void testBadParameters() {
    TS_ASSERT_THROWS(Progress(nullptr, -1., 100., 42), const std::invalid_argument &);
    TS_ASSERT_THROWS(Progress(nullptr, 1., .1, 42), const std::invalid_argument &);
  }
};

#endif
