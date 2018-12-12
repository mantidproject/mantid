// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_SKIPPINGPOLICYTEST_H_
#define MANTID_DATAOBJECTS_SKIPPINGPOLICYTEST_H_

#include "MantidDataObjects/SkippingPolicy.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;

class SkippingPolicyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SkippingPolicyTest *createSuite() { return new SkippingPolicyTest(); }
  static void destroySuite(SkippingPolicyTest *suite) { delete suite; }

  void test_SkipNothing() {
    SkipNothing skipNothing;
    SkippingPolicy &p = skipNothing;
    TSM_ASSERT_EQUALS("Should alway return False", false, p.keepGoing());
  }
};

#endif /* MANTID_DATAOBJECTS_SKIPPINGPOLICYTEST_H_ */
