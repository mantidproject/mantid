// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTPLOTOPTIONSMODELTEST_H_
#define MANTIDQT_INDIRECTPLOTOPTIONSMODELTEST_H_

#include <cxxtest/TestSuite.h>

class IndirectPlotOptionsModelTest : public CxxTest::TestSuite {
public:
  static IndirectPlotOptionsModelTest *createSuite() {
    return new IndirectPlotOptionsModelTest();
  }

  static void destroySuite(IndirectPlotOptionsModelTest *suite) {
    delete suite;
  }

  void setUp() override {}

  void tearDown() override {}

  void test_that_first_test() {}
};

#endif /* MANTIDQT_INDIRECTPLOTOPTIONSMODELTEST_H_ */
