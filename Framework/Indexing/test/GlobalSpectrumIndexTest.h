// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/GlobalSpectrumIndex.h"

using namespace Mantid;
using namespace Indexing;

class GlobalSpectrumIndexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GlobalSpectrumIndexTest *createSuite() { return new GlobalSpectrumIndexTest(); }
  static void destroySuite(GlobalSpectrumIndexTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    GlobalSpectrumIndex data(0);
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG((dynamic_cast<detail::IndexType<GlobalSpectrumIndex, size_t> &>(data))));
  }
};
