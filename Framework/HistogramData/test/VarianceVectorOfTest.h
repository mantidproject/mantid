#ifndef MANTID_HISTOGRAMDATA_VARIANCEVECTOROFTEST_H_
#define MANTID_HISTOGRAMDATA_VARIANCEVECTOROFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/VarianceVectorOf.h"

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::VarianceVectorOf;
using Mantid::HistogramData::detail::VectorOf;

class SigmasTester : public VectorOf<SigmasTester, HistogramX>,
                     public Iterable<SigmasTester> {
public:
  using VectorOf<SigmasTester, HistogramX>::VectorOf;
  using VectorOf<SigmasTester, HistogramX>::operator=;
};

class VarianceVectorOfTester
    : public VarianceVectorOf<VarianceVectorOfTester, HistogramX,
                              SigmasTester> {
public:
  using VarianceVectorOf<VarianceVectorOfTester, HistogramX,
                         SigmasTester>::VarianceVectorOf;
  using VarianceVectorOf<VarianceVectorOfTester, HistogramX, SigmasTester>::
  operator=;
  VarianceVectorOfTester(const VarianceVectorOfTester &) = default;
  VarianceVectorOfTester &operator=(const VarianceVectorOfTester &) = default;
};

class VarianceVectorOfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VarianceVectorOfTest *createSuite() {
    return new VarianceVectorOfTest();
  }
  static void destroySuite(VarianceVectorOfTest *suite) { delete suite; }

  void test_copy_construct() {
    VarianceVectorOfTester other{1.0, 2.0};
    VarianceVectorOfTester variances(other);
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 2.0);
  }

  void test_assign() {
    VarianceVectorOfTester other{1.0, 2.0};
    VarianceVectorOfTester variances{};
    variances = other;
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 2.0);
  }

  void test_construct_from_Sigmas() {
    const SigmasTester sigmas{1.0, 2.0};
    VarianceVectorOfTester variances(sigmas);
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 4.0);
  }

  void test_move_construct_from_Sigmas() {
    SigmasTester sigmas{1.0, 2.0};
    auto old_ptr = &sigmas[0];
    VarianceVectorOfTester variances(std::move(sigmas));
    TS_ASSERT(!sigmas);
    TS_ASSERT_EQUALS(&variances[0], old_ptr);
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 4.0);
  }

  void test_assign_Sigmas() {
    const SigmasTester sigmas{1.0, 2.0};
    VarianceVectorOfTester variances{};
    variances = sigmas;
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 4.0);
  }

  void test_move_assign_Sigmas() {
    SigmasTester sigmas{1.0, 2.0};
    auto old_ptr = &sigmas[0];
    VarianceVectorOfTester variances{};
    variances = std::move(sigmas);
    TS_ASSERT(!sigmas);
    TS_ASSERT_EQUALS(&variances[0], old_ptr);
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 4.0);
  }
};

#endif /* MANTID_HISTOGRAMDATA_VARIANCEVECTOROFTEST_H_ */
