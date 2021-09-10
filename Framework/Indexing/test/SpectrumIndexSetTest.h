// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/SpectrumIndexSet.h"

using namespace Mantid;
using namespace Indexing;

class SpectrumIndexSetTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumIndexSetTest *createSuite() { return new SpectrumIndexSetTest(); }
  static void destroySuite(SpectrumIndexSetTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    SpectrumIndexSet data(0);
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG((dynamic_cast<detail::IndexSet<SpectrumIndexSet> &>(data))));
  }
};
