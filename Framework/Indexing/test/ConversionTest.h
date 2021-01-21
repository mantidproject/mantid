// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Conversion.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/SpectrumNumber.h"

using namespace Mantid::Indexing;

class ConversionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConversionTest *createSuite() { return new ConversionTest(); }
  static void destroySuite(ConversionTest *suite) { delete suite; }

  void test_just_a_static_cast() {
    std::vector<SpectrumNumber> in{-1};
    TS_ASSERT_EQUALS(castVector<size_t>(in), (std::vector<size_t>{18446744073709551615ul}));
  }

  void test_GlobalSpectrumIndex() {
    std::vector<GlobalSpectrumIndex> in{0, 1, 2};
    TS_ASSERT_EQUALS(castVector<size_t>(in), (std::vector<size_t>{0, 1, 2}));
    TS_ASSERT_EQUALS(castVector<int64_t>(in), (std::vector<int64_t>{0, 1, 2}));
    TS_ASSERT_EQUALS(castVector<int32_t>(in), (std::vector<int32_t>{0, 1, 2}));
  }

  void test_to_GlobalSpectrumIndex() {
    std::vector<int64_t> in{0, 1, 2};
    TS_ASSERT_EQUALS(castVector<GlobalSpectrumIndex>(in), (std::vector<GlobalSpectrumIndex>{0, 1, 2}));
    std::vector<size_t> in2{0, 1, 2};
    TS_ASSERT_EQUALS(castVector<GlobalSpectrumIndex>(in2), (std::vector<GlobalSpectrumIndex>{0, 1, 2}));
  }

  void test_SpectrumNumber() {
    std::vector<SpectrumNumber> in{-1, 1, 2};
    TS_ASSERT_EQUALS(castVector<int64_t>(in), (std::vector<int64_t>{-1, 1, 2}));
    TS_ASSERT_EQUALS(castVector<int32_t>(in), (std::vector<int32_t>{-1, 1, 2}));
  }

  void test_to_SpectrumNumber() {
    std::vector<int32_t> in{-1, 1, 2};
    TS_ASSERT_EQUALS(castVector<SpectrumNumber>(in), (std::vector<SpectrumNumber>{-1, 1, 2}));
    std::vector<int64_t> in2{-1, 1, 2};
    TS_ASSERT_EQUALS(castVector<SpectrumNumber>(in2), (std::vector<SpectrumNumber>{-1, 1, 2}));
  }
};
