#ifndef MANTID_HISTOGRAMDATA_STANDARDDEVIATIONVECTOROFTEST_H_
#define MANTID_HISTOGRAMDATA_STANDARDDEVIATIONVECTOROFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::StandardDeviationVectorOf;
using Mantid::HistogramData::detail::VectorOf;

class VariancesTester : public VectorOf<VariancesTester, HistogramX>,
                        public Iterable<VariancesTester> {
public:
  using VectorOf<VariancesTester, HistogramX>::VectorOf;
  using VectorOf<VariancesTester, HistogramX>::operator=;
};

class StandardDeviationVectorOfTester
    : public StandardDeviationVectorOf<StandardDeviationVectorOfTester,
                                       HistogramX, VariancesTester> {
public:
  using StandardDeviationVectorOf<StandardDeviationVectorOfTester, HistogramX,
                                  VariancesTester>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<StandardDeviationVectorOfTester, HistogramX,
                                  VariancesTester>::operator=;
  StandardDeviationVectorOfTester(const StandardDeviationVectorOfTester &) =
      default;
  StandardDeviationVectorOfTester &
  operator=(const StandardDeviationVectorOfTester &) = default;
};

class StandardDeviationVectorOfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StandardDeviationVectorOfTest *createSuite() {
    return new StandardDeviationVectorOfTest();
  }
  static void destroySuite(StandardDeviationVectorOfTest *suite) {
    delete suite;
  }

  void test_copy_construct() {
    StandardDeviationVectorOfTester other{1.0, 2.0};
    StandardDeviationVectorOfTester variances(other);
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 2.0);
  }

  void test_assign() {
    StandardDeviationVectorOfTester other{1.0, 2.0};
    StandardDeviationVectorOfTester variances{};
    variances = other;
    TS_ASSERT_EQUALS(variances[0], 1.0);
    TS_ASSERT_EQUALS(variances[1], 2.0);
  }

  void test_construct_from_Variances() {
    const VariancesTester variances{1.0, 4.0};
    StandardDeviationVectorOfTester sigmas(variances);
    TS_ASSERT_EQUALS(sigmas[0], 1.0);
    TS_ASSERT_EQUALS(sigmas[1], 2.0);
  }

  void test_move_construct_from_Variances() {
    VariancesTester variances{1.0, 4.0};
    auto old_ptr = &variances[0];
    StandardDeviationVectorOfTester sigmas(std::move(variances));
    TS_ASSERT(!variances);
    TS_ASSERT_EQUALS(&sigmas[0], old_ptr);
    TS_ASSERT_EQUALS(sigmas[0], 1.0);
    TS_ASSERT_EQUALS(sigmas[1], 2.0);
  }

  void test_assign_Variances() {
    const VariancesTester variances{1.0, 4.0};
    StandardDeviationVectorOfTester sigmas{};
    sigmas = variances;
    TS_ASSERT_EQUALS(sigmas[0], 1.0);
    TS_ASSERT_EQUALS(sigmas[1], 2.0);
  }

  void test_move_assign_Variances() {
    VariancesTester variances{1.0, 4.0};
    auto old_ptr = &variances[0];
    StandardDeviationVectorOfTester sigmas{};
    sigmas = std::move(variances);
    TS_ASSERT(!variances);
    TS_ASSERT_EQUALS(&sigmas[0], old_ptr);
    TS_ASSERT_EQUALS(sigmas[0], 1.0);
    TS_ASSERT_EQUALS(sigmas[1], 2.0);
  }
};

#endif /* MANTID_HISTOGRAMDATA_STANDARDDEVIATIONVECTOROFTEST_H_ */
