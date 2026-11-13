// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <concepts>

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Multipliable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"

using namespace Mantid;
using namespace HistogramData;

class HistogramYTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramYTest *createSuite() { return new HistogramYTest(); }
  static void destroySuite(HistogramYTest *suite) { delete suite; }

  // The implementation of the testee is based on mixins and a few
  // compiler-generated constructors. Tests for the features provided by mixins
  // can be found in the respective mixin tests. Here we simply test that we
  // have the expected mixins.

  void test_has_correct_mixins() {
    TS_ASSERT((std::derived_from<HistogramY, detail::FixedLengthVector<HistogramY>>));
    TS_ASSERT((std::derived_from<HistogramY, detail::Addable<HistogramY>>));
    TS_ASSERT((std::derived_from<HistogramY, detail::Offsetable<HistogramY>>));
    TS_ASSERT((std::derived_from<HistogramY, detail::Multipliable<HistogramY>>));
    TS_ASSERT((std::derived_from<HistogramY, detail::Scalable<HistogramY>>));
  }
};
