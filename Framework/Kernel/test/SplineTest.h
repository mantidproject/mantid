// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/Spline.h"
#include <cxxtest/TestSuite.h>

#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

using namespace Mantid::Kernel;
using Cubic = Mantid::Kernel::CubicSpline<double, double>;
using Linear = Mantid::Kernel::LinearSpline<double, double>;

class SplineTest : public CxxTest::TestSuite {
public:
  static SplineTest *createSuite() { return new SplineTest(); }
  static void destroySuite(SplineTest *suite) { delete suite; }

  std::default_random_engine generator;

  SplineTest() {
    // prepare a random number generator for tests
    generator = std::default_random_engine();
  }

  void

      void
      test_cspline_basic() {
    // range of test data
    double x0 = 0.0, x1 = 1.0;

    // create the data -- sixth-order polynomial of random coefficients
    std::size_t const data_order = 6;
    // generate random coefficients
    double y0 = -5.0, y1 = 7.2;
    std::uniform_real_distribution<double> coefficient_maker(y0, y1);
    double A[data_order];
    for (std::size_t i = 0; i < data_order; i++) {
      A[i] = coefficient_maker(this->generator);
    }
    std::function<double(double)> polynomial = [A, data_order](double x) -> double {
      double y = A[0];
      for (std::size_t i = 1; i < data_order; i++) {
        y += A[i] * pow(x, i);
      }
      return y;
    };
    // create the x and y data
    std::size_t const N = 15;
    double delta = (x1 - x0) / (N - 1);
    std::vector<double> xtest(N), ytest(N);
    for (std::size_t k = 0; k < N; k++) {
      xtest[k] = static_cast<double>(k) * delta;
      ytest[k] = polynomial(xtest[k]);
    }

    // create a cubic spline
    Cubic cspline(xtest, ytest);

    // ensure that the splines fit the data
    for (std::size_t k = 0; k < N; k++) {
      TS_ASSERT_EQUALS(cspline(xtest[k]), ytest[k]);
    }

    // test that the splines interpolate the data
    std::size_t count = 0;
    double dx = delta / 7.0;
    double sumc = 0.;
    for (double x = x0; x <= x1; x += dx) {
      double y = polynomial(x);
      sumc += fabs(y - cspline(x)) / fabs(y);
      count++;
    }
    sumc /= count;
    TS_ASSERT_LESS_THAN(sumc, 1. / N);
  }

  void test_lspline_basic() {
    // range of test data
    double x0 = 0.0, x1 = 1.0;

    // create the data -- sixth-order polynomial of random coefficients
    std::size_t const data_order = 6;
    // generate random coefficients
    double y0 = -5.0, y1 = 7.2;
    std::uniform_real_distribution<double> coefficient_maker(y0, y1);
    double A[data_order];
    for (std::size_t i = 0; i < data_order; i++) {
      A[i] = coefficient_maker(this->generator);
    }
    std::function<double(double)> polynomial = [A, data_order](double x) -> double {
      double y = A[0];
      for (std::size_t i = 1; i < data_order; i++) {
        y += A[i] * pow(x, i);
      }
      return y;
    };
    // create the x and y data
    std::size_t const N = 15;
    double delta = (x1 - x0) / (N - 1);
    std::vector<double> xtest(N), ytest(N);
    for (std::size_t k = 0; k < N; k++) {
      xtest[k] = static_cast<double>(k) * delta;
      ytest[k] = polynomial(xtest[k]);
    }

    // create a linear spline
    Linear lspline(xtest, ytest);

    // ensure that the splines fit the data
    for (std::size_t k = 0; k < N; k++) {
      TS_ASSERT_EQUALS(lspline(xtest[k]), ytest[k]);
    }

    // test that the splines interpolate the data
    std::size_t count = 0;
    double dx = delta / 7.0;
    double suml = 0.;
    for (double x = x0; x <= x1; x += dx) {
      double y = polynomial(x);
      suml += fabs(y - lspline(x)) / fabs(y);
      count++;
    }
    suml /= count;
    TS_ASSERT_LESS_THAN(suml, 1. / N);
  }

  void test_cspline_line() {
    // test distribution, line of slope 1
    std::size_t const num_points = 23;
    double const x0 = 0.0, x1 = 12.0, Dx = (x1 - x0) / (num_points - 1);
    double const slope = 2.1, intercept = 0.3;
    std::vector<double> xtest(num_points), ytest(num_points);
    std::function<double(double)> line = [slope, intercept](double x) -> double { return slope * x + intercept; };
    for (std::size_t i = 0; i < num_points; i++) {
      xtest[i] = static_cast<double>(i) * Dx;
      ytest[i] = line(xtest[i]);
    }

    // spline fit the line
    Cubic cspline(xtest, ytest);

    // run through the x-axis and check the fit
    // a natural cubic spline can exactly fit a line
    double dx = 0.01;
    for (double x = 0.0; x <= x1; x += dx) {
      TS_ASSERT_DELTA(line(x), cspline(x), 1.e-10);
      TS_ASSERT_DELTA(slope, cspline.deriv(x), 1.e-10);
    }

    // pick random spots on the x-axis, and ensure exact fit
    std::uniform_real_distribution<double> sample_x_axis(x0, x1);
    double x = 0.0;
    for (int I = 0; I < 100; I++) {
      x = sample_x_axis(this->generator);
      TS_ASSERT_DELTA(line(x), cspline(x), 1.e-10);
      TS_ASSERT_DELTA(slope, cspline.deriv(x), 1.e-10);
    }
  }

  void test_lspline_line() {
    // test distribution, line of slope 1
    std::size_t const num_points = 23;
    double const x0 = 0.0, x1 = 12.0, Dx = (x1 - x0) / (num_points - 1);
    double const slope = 2.1, intercept = 0.3;
    std::vector<double> xtest(num_points), ytest(num_points);
    std::function<double(double)> line = [slope, intercept](double x) -> double { return slope * x + intercept; };
    for (std::size_t i = 0; i < num_points; i++) {
      xtest[i] = static_cast<double>(i) * Dx;
      ytest[i] = line(xtest[i]);
    }

    // spline fit the line
    Linear lspline(xtest, ytest);

    // run through the x-axis and check the fit
    // a natural cubic spline can exactly fit a line
    double dx = 0.01;
    for (double x = 0.0; x <= x1; x += dx) {
      TS_ASSERT_DELTA(line(x), lspline(x), 1.e-10);
      TS_ASSERT_DELTA(slope, lspline.deriv(x), 1.e-10);
    }

    // pick random spots on the x-axis, and ensure exact fit
    std::uniform_real_distribution<double> sample_x_axis(x0, x1);
    double x = 0.0;
    for (int I = 0; I < 100; I++) {
      x = sample_x_axis(this->generator);
      TS_ASSERT_DELTA(line(x), lspline(x), 1.e-10);
      TS_ASSERT_DELTA(slope, lspline.deriv(x), 1.e-10);
    }
  }

  void test_cspline_quadratic() {
    double x0 = 0.0, x1 = 10.0;
    std::uniform_real_distribution<double> sample_x_axis(x0, x1);

    // number of data points, for testing error scaling
    std::size_t const num_data[4] = {100, 200, 400, 800};

    // the quadratic function to be fit
    double A = 1.2, B = 0.7, C = 2.3;
    std::function<double(double)> quadratic = [A, B, C](double x) -> double { return (A * x + B) * x + C; };

    double resid[4];
    std::vector<double> xtest, ytest;
    double deltax;
    for (int I = 0; I < 4; I++) {
      // create the fit data
      xtest.resize(num_data[I] + 1);
      ytest.resize(num_data[I] + 1);
      deltax = (x1 - x0) / (num_data[I]);
      for (int i = 0; i < num_data[I] + 1; i++) {
        xtest[i] = i * deltax;
        ytest[i] = quadratic(xtest[i]);
      }
      // create the spline fit
      Cubic cspline(xtest, ytest);

      // now test the interpolaton
      // create a summed residual between the fit and function
      double dx = deltax / 8.0;
      double sum_fit = 0.0;
      std::size_t count = 0;
      for (double x = 1.0; x < 9.0; x += dx) {
        TS_ASSERT_DELTA(quadratic(x), cspline(x), 1.e-8);
        sum_fit += fabs(quadratic(x) - cspline(x)) / fabs(quadratic(x));
        count++;
      }
      sum_fit /= count;
      resid[I] = sum_fit;
      xtest.clear();
      ytest.clear();
    }

    // assert that the summed residual is decreasing with number of points
    for (int I = 1; I < 4; I++) {
      TS_ASSERT_LESS_THAN(resid[I], fmax(1.e-15, resid[I - 1]));
    }
  }

  void test_lspline_quadratic() {
    double x0 = 0.0, x1 = 10.0;
    std::uniform_real_distribution<double> sample_x_axis(x0, x1);

    // number of data points, for testing error scaling
    std::size_t const num_data[4] = {100, 200, 400, 800};

    // the quadratic function to be fit
    double A = 1.2, B = 0.7, C = 2.3;
    std::function<double(double)> quadratic = [A, B, C](double x) -> double { return (A * x + B) * x + C; };

    double resid[4];
    std::vector<double> xtest, ytest;
    double deltax;
    for (int I = 0; I < 4; I++) {
      // create the fit data
      xtest.resize(num_data[I] + 1);
      ytest.resize(num_data[I] + 1);
      deltax = (x1 - x0) / (num_data[I]);
      for (int i = 0; i < num_data[I] + 1; i++) {
        xtest[i] = i * deltax;
        ytest[i] = quadratic(xtest[i]);
      }
      // create the spline fit
      Linear lspline(xtest, ytest);

      // now test the interpolaton
      // create a summed residual between the fit and function
      double dx = deltax / 8.0;
      double sum_fit = 0.0;
      std::size_t count = 0;
      for (double x = 1.0; x < 9.0; x += dx) {
        sum_fit += fabs(quadratic(x) - lspline(x)) / fabs(quadratic(x));
        count++;
      }
      sum_fit /= count;
      resid[I] = sum_fit;
      xtest.clear();
      ytest.clear();
    }

    // assert that the summed residual is decreasing with number of points
    for (int I = 1; I < 4; I++) {
      TS_ASSERT_LESS_THAN(resid[I], fmax(1.e-15, resid[I - 1]));
    }
  }

  void test_cspline_cubic() {
    double A = 1.2, B = 0.5, C = 3.0, D = -5.0;
    std::function<double(double)> cubic = [A, B, C, D](double x) -> double { return ((A * x + B) * x + C) * x + D; };

    int const num_data = 100;
    double xtest[num_data], ytest[num_data];
    double xstart = 0.0, xstop = 10.0;
    double deltax = (xstop - xstart) / (num_data - 1);

    for (int i = 0; i < num_data; i++) {
      xtest[i] = i * deltax;
      ytest[i] = cubic(xtest[i]);
    }

    Cubic spline(xtest, ytest);

    double dx = deltax / 7.0;
    double sum_fit = 0.0, sum_deriv = 0.0;
    std::size_t count = 0;
    for (double x = 1.0; x < 9.0; x += dx) {
      sum_fit += fabs(cubic(x) - spline(x)) / fabs(cubic(x));
      double yprime = (3. * A * x + 2. * B) * x + C;
      sum_deriv += fabs(yprime - spline.deriv(x)) / fabs(yprime);
      count++;
    }
    sum_fit /= count;
    sum_deriv /= count;
    TS_ASSERT_LESS_THAN(sum_fit, 1.e-8);
    TS_ASSERT_LESS_THAN(sum_deriv, 1.e-8);
  }
};
