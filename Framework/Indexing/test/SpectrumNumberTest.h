// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/SpectrumNumber.h"

using namespace Mantid;
using namespace Indexing;

class SpectrumNumberTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumNumberTest *createSuite() { return new SpectrumNumberTest(); }
  static void destroySuite(SpectrumNumberTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    SpectrumNumber data(0);
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG((dynamic_cast<detail::IndexType<SpectrumNumber, int32_t> &>(data))));
  }
};
