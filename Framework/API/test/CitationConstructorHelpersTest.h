// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef CITATIONCONSTRUCTORHELPERSTEST_H_
#define CITATIONCONSTRUCTORHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

class CitationConstructorHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CitationConstructorHelpersTest *createSuite() {
    return new CitationConstructorHelpersTest();
  }
  static void destroySuite(CitationConstructorHelpersTest *suite) {
    delete suite;
  }

  void test_something() {}
};

#endif /* CITATIONCONSTRUCTORHELPERSTEST_H_ */