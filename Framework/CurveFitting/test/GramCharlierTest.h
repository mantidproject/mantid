#ifndef MANTID_CURVEFITTING_GRAMCHARLIERTEST_H_
#define MANTID_CURVEFITTING_GRAMCHARLIERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/GramCharlier.h"
#include <array>

using Mantid::CurveFitting::Functions::GramCharlier;

class GramCharlierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GramCharlierTest *createSuite() { return new GramCharlierTest(); }
  static void destroySuite(GramCharlierTest *suite) { delete suite; }

  void test_with_all_terms() {
    auto input = createTestInput<11>(-0.5);

    GramCharlier fn;
    fn.initialize();
    fn.setParameter("A", 0.01);
    fn.setParameter("X0", 0.2);
    fn.setParameter("Sigma", 4);
    fn.setParameter("C4", -0.005);
    fn.setParameter("C6", -0.003);
    fn.setParameter("C8", -0.002);
    fn.setParameter("C10", -0.001);
    fn.setParameter("Afse", 0.005);

    decltype(input) output;
    fn.function1D(output.data(), input.data(), input.size());

    TSM_ASSERT_EQUALS("Input/output data sizes should match", output.size(),
                      input.size());
    // Values computed independently with UserFunction in MantidPlot
    decltype(input) expected = {
        {0.00132130149415442, 0.00127865974554395, 0.0012345081447521,
         0.00118898601238179, 0.00114223710903171, 0.00109440900868656,
         0.00104565245357959, 0.000996120694139449, 0.000945968817727083,
         0.000895353069936715, 0.000844430172276783}};
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
    const double precision(1e-10);
    for (size_t i = 0; i < expected.size(); ++i) {
      TSM_ASSERT_DELTA("Mismatch in " + std::to_string(i) + "th element",
                       expected[i], computed[i], precision);
    }
  }
};

#endif /* MANTID_CURVEFITTING_GRAMCHARLIERTEST_H_ */
