// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_TOPICINFOTEST_H_
#define MANTID_KERNEL_TOPICINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TopicInfo.h"

using Mantid::Kernel::TopicInfo;

class TopicInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TopicInfoTest *createSuite() { return new TopicInfoTest(); }
  static void destroySuite(TopicInfoTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_KERNEL_TOPICINFOTEST_H_ */