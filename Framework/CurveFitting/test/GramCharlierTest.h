#ifndef MANTID_CURVEFITTING_GRAMCHARLIERTEST_H_
#define MANTID_CURVEFITTING_GRAMCHARLIERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/GramCharlier.h"

using Mantid::CurveFitting::Functions::GramCharlier;

class GramCharlierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GramCharlierTest *createSuite() { return new GramCharlierTest(); }
  static void destroySuite(GramCharlierTest *suite) { delete suite; }

  void test_default_includes_all_terms() {
    auto input = createTestInput<11>(-0.5);
    decltype(input) output;

    GramCharlier fn;
    fn.initialize();
    fn.function1D(output.data(), input.data(), input.size());
    TSM_ASSERT_EQUALS("Input/output data sizes should match", output.size(),
                      input.size());
    decltype(input) expected = {
        0.00098105527174162,  0.000985033719286382, 0.000988412700063565,
        0.000991185942632468, 0.000993348288951089, 0.000994895710427353,
        0.000995825320477443, 0.000996135383535863, 0.000995825320477443,
        0.000994895710427353, 0.000993348288951089};
    checkMatch(expected, output);
  }

private:
  template <size_t N> std::array<double, N> createTestInput(double start) {
    const double step(0.1);
    std::array<double, N> data;
    std::generate(std::begin(data), std::end(data), [&start, step] {
      double tmp(start);
      start += step;
      return tmp;
    });
    return data;
  }

  /// Verify that the sequences match within some precision. It is assumed
  /// that the sizes match on entry
  template <typename ExpectedSeqType, typename ComputedSeqType>
  void checkMatch(const ExpectedSeqType &expected,
                  const ComputedSeqType &computed) {
    const double precision(1e-08);
    for (size_t i = 0; i < expected.size(); ++i) {
      TSM_ASSERT_DELTA("Mismatch in " + std::to_string(i) + "th element",
                       expected[i], computed[i], precision);
    }
  }
};

#endif /* MANTID_CURVEFITTING_GRAMCHARLIERTEST_H_ */
