// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_ESTIMATEPOLYNOMIALTEST_H_
#define MANTID_HISTOGRAMDATA_ESTIMATEPOLYNOMIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/QuadraticGenerator.h"

using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::QuadraticGenerator;
using Mantid::HistogramData::estimateBackground;
using Mantid::HistogramData::estimatePolynomial;

class EstimatePolynomialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimatePolynomialTest *createSuite() {
    return new EstimatePolynomialTest();
  }
  static void destroySuite(EstimatePolynomialTest *suite) { delete suite; }

  void test_BadParameters() {
    Histogram histo(Points{10, LinearGenerator(0., 1.)},
                    Counts{10, LinearGenerator(10., 0.)});

    double bg0, bg1, bg2, chisq;
    // bad order
    TS_ASSERT_THROWS(Mantid::HistogramData::estimatePolynomial(
                         3, histo, 0, histo.size(), bg0, bg1, bg2, chisq),
                     const std::runtime_error &);
    // bad range i_max < i_min
    TS_ASSERT_THROWS(estimatePolynomial(2, histo, 1, 0, bg0, bg1, bg2, chisq),
                     const std::runtime_error &);
    // bad range x.size() < i_max
    TS_ASSERT_THROWS(estimatePolynomial(2, histo, 0, 30, bg0, bg1, bg2, chisq),
                     const std::runtime_error &);
  }

  void test_FlatData() {
    Histogram histo(Points{10, LinearGenerator(0., 1.)},
                    Counts{10, LinearGenerator(10., 0.)});

    double bg0, bg1, bg2, chisq;
    for (size_t order = 0; order < 3;
         ++order) { // should always return that constant is best
      estimatePolynomial(order, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
      TS_ASSERT_EQUALS(bg0, 10.);
      TS_ASSERT_EQUALS(bg1, 0.);
      TS_ASSERT_EQUALS(bg2, 0.);
    }
  }

  void test_LinearData() {
    Histogram histo(Points{10, LinearGenerator(0., 1.)},
                    Counts{10, LinearGenerator(0., 12.)});

    double bg0, bg1, bg2, chisq;

    // flat
    std::cout << "*** *** order=" << 0 << std::endl;
    estimatePolynomial(0, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
    std::cout << "chisq=" << chisq << std::endl;
    TS_ASSERT_DELTA(bg0, 54., .00001);
    TS_ASSERT_EQUALS(bg1, 0.);
    TS_ASSERT_EQUALS(bg2, 0.);

    // linear
    std::cout << "*** *** order=" << 1 << std::endl;
    estimatePolynomial(1, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
    std::cout << "chisq=" << chisq << std::endl;
    TS_ASSERT_EQUALS(bg0, 0.);
    TS_ASSERT_EQUALS(bg1, 12.);
    TS_ASSERT_EQUALS(bg2, 0.);

    // quadratic
    std::cout << "*** *** order=" << 2 << std::endl;
    estimatePolynomial(2, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
    std::cout << "chisq=" << chisq << std::endl;
    TS_ASSERT_EQUALS(bg0, 0.);
    TS_ASSERT_EQUALS(bg1, 12.);
    TS_ASSERT_EQUALS(bg2, 0.);
  }

  void test_QuadraticData() {
    Histogram histo(Points{10, LinearGenerator(0., 1.)},
                    Counts{10, QuadraticGenerator(10., 12., -3.)});
    std::cout << "-> quad: ";
    for (const auto &val : histo.y())
      std::cout << val << " ";
    std::cout << std::endl;

    double bg0, bg1, bg2, chisq;

    // flat
    std::cout << "*** *** order=" << 0 << std::endl;
    estimatePolynomial(0, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
    std::cout << "chisq=" << chisq << std::endl;
    TS_ASSERT_DELTA(bg0, -21.5, .00001);
    TS_ASSERT_EQUALS(bg1, 0.);
    TS_ASSERT_EQUALS(bg2, 0.);

    // linear
    std::cout << "*** *** order=" << 1 << std::endl;
    estimatePolynomial(1, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
    std::cout << "chisq=" << chisq << std::endl;
    TS_ASSERT_DELTA(bg0, 46., .00001);
    TS_ASSERT_DELTA(bg1, -15., .00001);
    TS_ASSERT_EQUALS(bg2, 0.);

    // quadratic
    std::cout << "*** *** order=" << 2 << std::endl;
    estimatePolynomial(2, histo, 0, histo.size(), bg0, bg1, bg2, chisq);
    std::cout << "chisq=" << chisq << std::endl;
    TS_ASSERT_EQUALS(bg0, 10.);
    TS_ASSERT_EQUALS(bg1, 12.);
    TS_ASSERT_EQUALS(bg2, -3.);
  }
};

#endif /* MANTID_HISTOGRAMDATA_ESTIMATEPOLYNOMIALTEST_H_ */
