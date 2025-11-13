// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/Smoothing.h"
#include <cxxtest/TestSuite.h>

#include <cmath>
#include <stdexcept>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Smoothing;

class SmoothingTest : public CxxTest::TestSuite {
public:
  static SmoothingTest *createSuite() { return new SmoothingTest(); }
  static void destroySuite(SmoothingTest *suite) { delete suite; }

  // BOX CAR SMOOTHING

  void test_boxcarSmooth_npoints_validation() {
    std::vector<double> input{1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (unsigned int npts = 0; npts < 3; npts++) {
      TS_ASSERT_THROWS(boxcarSmooth(input, npts), std::invalid_argument const &);
    }
    for (unsigned int npts = 1; npts < 3; npts++) {
      TS_ASSERT_THROWS(boxcarSmooth(input, 2 * npts), std::invalid_argument const &);
      TS_ASSERT_THROWS_NOTHING(boxcarSmooth(input, 2 * npts + 1));
    }
  }

  void test_boxcarSmooth_flat() {
    double const flatValue = 1.0;
    std::vector<double> input(20, flatValue);
    std::vector<double> output = boxcarSmooth(input, 3);
    TS_ASSERT_EQUALS(input, output);
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_boxcarSmooth_two() {
    // a series of values which should smooth out to 2
    std::vector<double> input{1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1};
    std::vector<double> output = boxcarSmooth(input, 3);
    output.pop_back(); // NOTE the last value can't ever equal 2
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, 2.0);
    }
  }

  void test_boxcarSmooth() {
    std::vector<double> yVals(10);
    for (int i = 0; i < 10; ++i) {
      yVals[i] = i + 1.0;
    }
    std::vector<double> Y = boxcarSmooth(yVals, 5);
    std::vector<double> expected{2, 2.5, 3, 4, 5, 6, 7, 8, 8.5, 9};

    TS_ASSERT_EQUALS(Y, expected);
  }

  // BOX CAR ERROR PROPAGATION

  void test_boxcarErrorSmooth_flat() {
    // NOTE this uses the error propagation equation, which tends to decrease the error values
    double const flatValue = 2.0; // NOTE using 2 to make sure square operation makes a different value
    std::vector<double> input(20, flatValue);
    std::vector<double> output = boxcarErrorSmooth(input, 3);
    TS_ASSERT_LESS_THAN(output, input);
    for (double const &x : output) {
      TS_ASSERT_LESS_THAN(x, flatValue);
    }
  }

  void test_boxcarErrorSmooth_two() {
    // a series of values which should sum-square-smooth out to 2
    double const a1 = 3., a2 = std::sqrt(7.), a3 = 2. * std::sqrt(5.);
    std::vector<double> input{a1, a2, a3, a1, a2, a3, a1, a2, a3, a1, a2, a3, a1};
    std::vector<double> output = boxcarErrorSmooth(input, 3);
    output.pop_back(); // NOTE the last value can't ever equal 2
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, 2.0);
    }
  }

  // BOX CAR RMSE SMOOTHING

  void test_boxcarRMSESmooth_flat() {
    double const flatValue = 2.0; // NOTE using 2 to make sure squaring changes value
    std::vector<double> input(20, flatValue);
    std::vector<double> output = boxcarRMSESmooth(input, 3);
    TS_ASSERT_EQUALS(input, output);
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_boxcarRMSESmooth_two() {
    // a series of values which should sum-square-smooth out to 2
    double const a1 = std::sqrt(3), a2 = std::sqrt(5.), a3 = 2.;
    std::vector<double> input{a1, a2, a3, a1, a2, a3, a1, a2, a3, a1, a2, a3, a1};
    std::vector<double> output = boxcarRMSESmooth(input, 3);
    output.pop_back(); // NOTE the last value can't ever equal 2
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, 2.0);
    }
  }

  // FFT SMOOTHING

  void test_fftSmooth_invalid() {
    int N = 10;
    int zero_cutoff = 0, large_cutoff = N + 1;
    std::vector<double> input(N, 1);
    TS_ASSERT_THROWS_ASSERT(fftSmooth(input, zero_cutoff), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "zero")));
    TS_ASSERT_THROWS_ASSERT(fftSmooth(input, large_cutoff), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "array size")));
  }

  void test_fftSmooth_flat() {
    // put flat signal in, get flat signal back out
    double const flatValue = 3.0;
    std::vector<double> input(20, flatValue);
    std::vector<double> output = fftSmooth(input, 10);
    TS_ASSERT_EQUALS(input, output);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_fftSmooth_spikey() {
    // put in flat signal with high-frequency noise, get flat out
    double const flatValue = 3.0;
    std::vector<double> input(20, flatValue);
    // add high-frequency spikes
    for (std::size_t i = 0; i < input.size(); i++) {
      input[i] += (i % 2 == 0 ? +1 : -1);
    }
    std::vector<double> output = fftSmooth(input, 1);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_fftSmooth_sines() {
    // put in low-freq sine data with high-freq noise; get low-freq sine out
    std::size_t const N{100};
    std::vector<double> input(N);
    // make periodic data
    double const w0 = 6.28318530717958648 / double(N);
    unsigned n1 = 3, n2 = 15;
    double w1 = n1 * w0, w2 = n2 * w0;
    auto sine = [](double w, std::size_t i) { return std::sin(w * double(i)); };
    for (std::size_t i = 0; i < N; i++) {
      input[i] = sine(w1, i) + sine(w2, i);
    }
    // cut off too low -- signal will be zero
    std::vector<double> output = fftSmooth(input, n1 - 1);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (std::size_t i = 0; i < output.size(); i++) {
      TS_ASSERT_DELTA(output[i], 0.0, 1.e-8);
    }
    // cutoff too high -- the higher frequency is still there
    output = fftSmooth(input, n2 + 1);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (std::size_t i = 0; i < output.size(); i++) {
      TS_ASSERT_DELTA(output[i], input[i], 1.e-8);
    }
    // cutoff just right -- the higher frequency part is removed
    output = fftSmooth(input, n2 - 1);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (std::size_t i = 0; i < output.size(); i++) {
      TS_ASSERT_DELTA(output[i], sine(w1, i), 1.e-8);
    }
  }

  void test_fftSmooth_gauss() {
    // put in gaussdata with high-freq noise; get gauss out
    std::size_t const N{100};
    std::vector<double> input(N);
    // make gaussian data with noise
    auto gauss = [](std::size_t i) { return std::exp(-std::pow(double(i) - 40.0, 2) / 15); };
    for (std::size_t i = 0; i < N; i++) {
      input[i] = gauss(i);
    }
    // add high-frequency noise
    for (std::size_t i = 0; i < input.size(); i++) {
      input[i] += (i % 2 == 0 ? +0.5 : -0.5);
    }
    // smooth and check
    std::vector<double> output = fftSmooth(input, 50);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (std::size_t i = 0; i < output.size(); i++) {
      TS_ASSERT_DELTA(output[i], gauss(i), 1.e-4);
    }
  }

  // butterworth

  void test_fftButterworthSmooth_invalid() {
    int N = 10;
    int zero_cutoff = 0, large_cutoff = N + 1, good_cutoff = N / 2;
    int zero_order = 0, good_order = 1;
    std::vector<double> input(N, 1);
    TS_ASSERT_THROWS_ASSERT(fftButterworthSmooth(input, zero_cutoff, good_order), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "zero")));
    TS_ASSERT_THROWS_ASSERT(fftButterworthSmooth(input, large_cutoff, good_order), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "array size")));
    TS_ASSERT_THROWS_ASSERT(fftButterworthSmooth(input, good_cutoff, zero_order), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "nonzero")));
  }

  void test_fftButterworthSmooth_flat() {
    // put flat signal in, get flat signal back out
    double const flatValue = 3.0;
    std::vector<double> input(20, flatValue);
    std::vector<double> output = fftButterworthSmooth(input, 1, 1);
    TS_ASSERT_EQUALS(input, output);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_fftButterworthSmooth_spikey() {
    // put in flat signal with high-frequency noise, get flat out
    double const flatValue = 3.0;
    std::vector<double> input(20, flatValue);
    // add high-frequency spikes
    for (std::size_t i = 0; i < input.size(); i++) {
      input[i] += (i % 2 == 0 ? +1 : -1);
    }
    std::vector<double> output = fftButterworthSmooth(input, 2, 10);
    TS_ASSERT_EQUALS(input.size(), output.size());
    for (double const &x : output) {
      TS_ASSERT_DELTA(x, flatValue, 1.e-4);
    }
  }
};
