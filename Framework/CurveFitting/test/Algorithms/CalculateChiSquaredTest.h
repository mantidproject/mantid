#ifndef MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_
#define MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/CalculateChiSquared.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/EmptyValues.h"

#include <cmath>
#include <algorithm>
#include <limits>

using Mantid::CurveFitting::Algorithms::CalculateChiSquared;
using namespace Mantid;
using namespace Mantid::API;

class CalculateChiSquaredTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateChiSquaredTest *createSuite() {
    return new CalculateChiSquaredTest();
  }
  static void destroySuite(CalculateChiSquaredTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void test_Init() {
    CalculateChiSquared alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_1D_empty_defaults() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumEmpty();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 20338.0, 1.0);
  }

  void test_1D_empty_all_x_range() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumEmpty();
    tester.setXRange_All();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 20338.0, 1.0);
  }

  void test_1D_empty_smaller() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumEmpty();
    tester.setXRange_Smaller();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 1189.0, 1.0);
  }

  void test_1D_empty_smaller1() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumEmpty();
    tester.setXRange_Smaller_bin_boundaries();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 1189.0, 1.0);
  }

  void test_1D_values() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumValues();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 1655.0, 1.0);
  }

  void test_1D_values_smaller() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumValues();
    tester.setXRange_Smaller();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 153.0, 1.0);
  }

  void test_1D_values_point_data() {
    Tester tester(3, 10, false);
    tester.set1DFunction();
    tester.set1DSpectrumValues();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 307.0, 1.0);
  }

  void test_1D_workspace_index() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumValues(5);
    tester.setXRange_Smaller();
    tester.setWorkspaceIndex();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 151.0, 1.0);
  }

  void test_1D_values_dont_ignore_invalid() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumValuesInvalid();
    tester.runAlgorithm();
    tester.checkFailed();
  }

  void test_1D_values_ignore_invalid() {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumValuesInvalid();
    tester.setIgnoreInvalidData();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 1450.39, 1.0);
  }

  void test_1D_values_divide_by_dof_fixed_params() {
    Tester tester(1);
    tester.set1DFunction();
    tester.set1DSpectrumValues();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 1655.0, 1.0);
  }

  void test_1D_values_divide_by_dof_zero() {
    Tester tester(3, 3);
    tester.set1DFunction();
    tester.set1DSpectrumValues();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 5069.0, 1.0);
  }

  void test_1D_values_divide_by_dof_negative() {
    Tester tester(3, 2);
    tester.set1DFunction();
    tester.set1DSpectrumValues();
    tester.runAlgorithm();
    tester.check1DSpectrum();
    TS_ASSERT_DELTA(tester.chiSquared, 7151.0, 1.0);
  }

  void test_errors_Thurber() {
    double x[] = {-3.067, -2.981, -2.921, -2.912, -2.840, -2.797, -2.702,
                  -2.699, -2.633, -2.481, -2.363, -2.322, -1.501, -1.460,
                  -1.274, -1.212, -1.100, -1.046, -0.915, -0.714, -0.566,
                  -0.545, -0.400, -0.309, -0.109, -0.103, 0.010,  0.119,
                  0.377,  0.790,  0.963,  1.006,  1.115,  1.572,  1.841,
                  2.047,  2.200};
    double y[] = {80.574,   84.248,   87.264,   87.195,   89.076,   89.608,
                  89.868,   90.101,   92.405,   95.854,   100.696,  101.060,
                  401.672,  390.724,  567.534,  635.316,  733.054,  759.087,
                  894.206,  990.785,  1090.109, 1080.914, 1122.643, 1178.351,
                  1260.531, 1273.514, 1288.339, 1327.543, 1353.863, 1414.509,
                  1425.208, 1421.384, 1442.962, 1464.350, 1468.705, 1447.894,
                  1457.628};
    std::string fun("name=UserFunction,Formula=(b1 + b2*x + b3*x^2 + b4*x^3) / "
                    "(1 + b5*x + b6*x^2 + b7*x^3),"
                    "b1=1.2881396800E+03, b2=1.4910792535E+03, "
                    "b3=5.8323836877E+02, b4=7.5416644291E+01, "
                    "b5=9.6629502864E-01, b6=3.9797285797E-01, "
                    "b7=4.9727297349E-02");
    double sigma[] = {4.6647963344E+00, 3.9571156086E+01, 2.8698696102E+01,
                      5.5675370270E+00, 3.1333340687E-02, 1.4984928198E-02,
                      6.5842344623E-03};
    size_t ndata = sizeof(x) / sizeof(double);
    Tester tester;
    tester.setTestCaseForErrorCalculations(ndata, x, y, fun);
    tester.runAlgorithm();
    tester.check1DSpectrum();
    tester.checkErrors(sigma);
  }

  void test_errors_Chwirut1() {
    double x[] = {
        0.5,   0.5,   0.5,  0.5,  0.5,  0.5,  0.5,  0.5,   0.5,   0.5,   0.5,
        0.5,   0.5,   0.5,  0.5,  0.5,  0.5,  0.5,  0.625, 0.625, 0.625, 0.625,
        0.625, 0.75,  0.75, 0.75, 0.75, 0.75, 0.75, 0.75,  0.75,  0.75,  0.75,
        0.75,  0.75,  0.75, 0.75, 0.75, 0.75, 0.75, 0.75,  0.875, 0.875, 0.875,
        0.875, 0.875, 1,    1,    1,    1,    1,    1,     1,     1,     1,
        1,     1,     1.25, 1.25, 1.25, 1.25, 1.25, 1.5,   1.5,   1.5,   1.5,
        1.5,   1.5,   1.5,  1.5,  1.5,  1.5,  1.5,  1.75,  1.75,  1.75,  1.75,
        1.75,  1.75,  1.75, 1.75, 1.75, 1.75, 1.75, 1.75,  1.75,  1.75,  1.75,
        1.75,  2,     2,    2,    2,    2,    2,    2,     2,     2,     2,
        2,     2,     2.25, 2.25, 2.25, 2.25, 2.25, 2.25,  2.25,  2.25,  2.25,
        2.25,  2.5,   2.5,  2.5,  2.5,  2.5,  2.5,  2.5,   2.5,   2.5,   2.75,
        2.75,  2.75,  2.75, 2.75, 2.75, 2.75, 3,    3,     3,     3,     3,
        3,     3,     3,    3,    3,    3,    3,    3,     3,     3,     3,
        3,     3,     3,    3,    3,    3,    3,    3,     3,     3,     3,
        3,     3,     3,    3.25, 3.25, 3.25, 3.25, 3.25,  3.75,  3.75,  3.75,
        3.75,  3.75,  3.75, 3.75, 4,    4,    4,    4,     4,     4,     4.25,
        4.25,  4.25,  4.25, 4.25, 4.75, 4.75, 4.75, 4.75,  4.75,  5,     5,
        5,     5,     5,    5,    5.25, 5.25, 5.25, 5.25,  5.25,  5.75,  5.75,
        5.75,  5.75,  5.75, 6,    6,    6,    6,    6,     6,     6,     6,
        6,     6,     6,    6,    6};
    double y[] = {
        76.8,    70.5,    78,     70.8,   90.6,    78,      74.1,    81.5,
        81.3,    92.9,    75.8,   66.7,   81,      81.7,    78,      80,
        80.7,    76.8,    79,     66,     76.9,    78.7,    67.3,    59.2,
        60.9,    62,      60,     63.5,   59.9,    61.6,    59.5,    61,
        63.8,    54.7,    71.6,   62,     61.3,    64.2,    62.4,    60.8,
        64.5,    55.5,    63.6,   57.2,   58,      64.9,    48.5,    47.8,
        48.8,    47.7,    53.2,   57.1,   40.8,    54,      47.5,    48,
        50.3,    39.2,    42.5,   43.3,   41,      37.8,    29,      35.5,
        35.2,    35.8,    32.9,   32,     32.5,    33.8,    39.8,    33.2,
        30.7,    29.3625, 29.4,   28.4,   29.175,  25.5,    31.1,    26.85,
        25.95,   28.9,    26.8,   28.95,  29.81,   29.3,    28.69,   29.8,
        31.05,   21.67,   21,     20.32,  20,      22.57,   22.2,    25.7,
        24,      29.8,    29.62,  25.99,  24.56,   21.15,   20.4,    23.6,
        22.125,  21.4,    20.4,   23.775, 21.07,   20.2,    21,      21,
        19.31,   16.95,   18.82,  16.3,   18.67,   17.7,    23.7,    23.81,
        16.7625, 17.5125, 17.17,  16.65,  16.4625, 17.7375, 13.87,   15.64,
        12.75,   13.95,   13.69,  15.64,  14.62,   15.19,   12.94,   13.12,
        12.75,   13.31,   12.56,  13.35,  13.31,   11.81,   16.24,   13.87,
        14.62,   17.7,    12.75,  13.84,  16.12,   12.07,   11.81,   18.07,
        25.76,   15.56,   24.19,  13.12,  12.41,   13.2,    14.25,   12.525,
        13.8,    9.67,    10.875, 9.45,   10.05,   10.39,   11.5875, 10.5375,
        7.76,    8.17,    10.42,  11.25,  8.62,    8.74,    11.55,   9.15,
        9.4125,  8.5875,  5.44,   8.175,  7.9125,  7.725,   7.125,   4.87,
        7.35,    7.31,    9,      7.2,    7.87,    12.07,   8.55,    7.35,
        6.1125,  4.01,    5.9625, 8.475,  5.9625,  3.75,    5.625,   6.1125,
        8.025,   7.42,    6.67,   5.06,   4.87,    5.44,    6.64,    5.63,
        5.44,    8.51,    8.74,   3.94,   5.63,    10.12};
    std::string fun("name=UserFunction,Formula=exp(-b1*x)/"
                    "(b2+b3*x),b1=1.9027818370E-01,b2=6.1314004477E-03,b3=1."
                    "0530908399E-02");
    double sigma[] = {2.1938557035E-02, 3.4500025051E-04, 7.9281847748E-04};
    size_t ndata = sizeof(x) / sizeof(double);
    Tester tester;
    tester.setTestCaseForErrorCalculations(ndata, x, y, fun);
    tester.runAlgorithm();
    tester.check1DSpectrum();
    tester.checkErrors(sigma);
  }

private:
  class Tester {
    // input parameters
    size_t nParams;
    size_t nData;
    bool isHisto;
    double xMin;
    double xMax;
    std::vector<double> xBins;
    std::vector<double> xValues;

    // values for algorithm input properties
    IFunction_sptr function;
    Workspace_sptr workspace;
    int workspaceIndex;
    double StartX;
    double EndX;
    bool ignoreInvalidData;
    std::string outputName;
    bool Weighted;

    void makeXValues() {
      size_t dlt = isHisto ? 1 : 0;
      xBins.resize(nData + dlt);
      double dx = (xMax - xMin) / double(xBins.size() - 1);
      for (size_t i = 0; i < xBins.size(); ++i) {
        xBins[i] = xMin + double(i) * dx;
      }
      if (isHisto) {
        xValues.resize(nData);
        std::transform(xBins.begin(), xBins.end() - 1, xValues.begin(),
                       std::bind2nd(std::plus<double>(), dx / 2));
      } else {
        xValues = xBins;
      }
    }

    void setDefaultXRange() {
      if (StartX == EMPTY_DBL())
        StartX = xMin;
      if (EndX == EMPTY_DBL()) {
        EndX = xMax;
      } else {
        auto ix = std::upper_bound(xBins.begin(), xBins.end(), EndX);
        if (ix != xBins.end())
          EndX = *ix;
        else
          EndX = xMax;
      }
    }

    bool isGoodValue(double y, double e) {
      return !ignoreInvalidData ||
             (std::isfinite(y) && std::isfinite(e) && e > 0);
    }

  public:
    // algorithm output
    double chiSquared;
    double chiSquaredDividedByDOF;
    double chiSquaredWeighted;
    double chiSquaredWeightedDividedByDOF;
    ITableWorkspace_sptr errors;
    ITableWorkspace_sptr pdfs;
    bool isExecuted;

    Tester(size_t np = 3, size_t nd = 10, bool histo = true)
        : nParams(np), nData(nd), isHisto(histo), xMin(-10), xMax(10),
          workspaceIndex(0), StartX(EMPTY_DBL()), EndX(EMPTY_DBL()),
          ignoreInvalidData(false), Weighted(false),
          // output
          chiSquared(-1), chiSquaredDividedByDOF(-1), chiSquaredWeighted(-1),
          chiSquaredWeightedDividedByDOF(-1), isExecuted(false) {
      makeXValues();
    }

    void runAlgorithm() {
      CalculateChiSquared alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Function", function));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", workspace));
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("IgnoreInvalidData", ignoreInvalidData));
      if (dynamic_cast<IFunction1D *>(function.get())) {
        TS_ASSERT_THROWS_NOTHING(
            alg.setProperty("WorkspaceIndex", workspaceIndex));
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartX", StartX));
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndX", EndX));
      }
      if (!outputName.empty()) {
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("Output", outputName));
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("Weighted", Weighted));
      }
      TS_ASSERT_THROWS_NOTHING(alg.execute());
      isExecuted = alg.isExecuted();
      if (isExecuted) {
        chiSquared = alg.getProperty("ChiSquared");
        chiSquaredDividedByDOF = alg.getProperty("ChiSquaredDividedByDOF");
        chiSquaredWeighted = alg.getProperty("ChiSquaredWeighted");
        chiSquaredWeightedDividedByDOF =
            alg.getProperty("ChiSquaredWeightedDividedByDOF");
        if (!outputName.empty()) {
          errors = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
              "out_errors");
          pdfs = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
              "out_pdf");
        }
      }
    }

    void setXRange_All() {
      StartX = xMin;
      EndX = xMax;
    }

    void setXRange_Smaller_bin_boundaries() {
      StartX = xBins[3];
      EndX = xBins[7];
    }

    void setXRange_Smaller() {
      StartX = xBins[3] - 0.3;
      EndX = xBins[7] + 0.7;
    }

    void setWorkspaceIndex() { workspaceIndex = 3; }

    void setIgnoreInvalidData() { ignoreInvalidData = true; }

    void
    set1DFunction(const std::string &fun =
                      "name=UserFunction,Formula=a+b*x+c*x^2,a=1,b=1,c=1") {
      function = FunctionFactory::Instance().createInitialized(fun);
      if (nParams < function->nParams()) {
        const size_t dn = function->nParams() - nParams;
        for (size_t i = 0; i < dn; ++i) {
          function->fix(i);
        }
      } else {
        TS_ASSERT_EQUALS(function->nParams(), nParams);
      }
    }

    void set1DSpectrumEmpty() {

      const size_t nSpec = 1;
      size_t dn = isHisto ? 1 : 0;
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec,
                                                       nData + dn, nData);
      space->dataX(0).assign(xBins.begin(), xBins.end());
      space->dataE(0).assign(nData, 10.0);
      workspace = space;
    }

    void set1DSpectrumValues(const size_t nSpec = 1) {
      size_t dn = isHisto ? 1 : 0;
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec,
                                                       nData + dn, nData);
      for (size_t spec = 0; spec < nSpec; ++spec) {
        space->dataX(spec).assign(xBins.begin(), xBins.end());
        for (size_t i = 0; i < nData; ++i) {
          const double x = space->readX(0)[i];
          space->dataY(spec)[i] =
              (1.1 + 0.1 * double(spec)) * (1.0 + x + x * x);
          space->dataE(spec)[i] = 10.0;
        }
      }
      workspace = space;
    }

    void set1DSpectrumValuesInvalid() {
      set1DSpectrumValues();
      auto &yValues =
          dynamic_cast<MatrixWorkspace &>(*workspace).dataY(workspaceIndex);
      yValues[2] = std::numeric_limits<double>::infinity();
      yValues[4] = std::numeric_limits<double>::quiet_NaN();
      auto &eValues =
          dynamic_cast<MatrixWorkspace &>(*workspace).dataE(workspaceIndex);
      eValues[6] = -1;
    }

    void setTestCaseForErrorCalculations(size_t ndata, double *xarray,
                                         double *yarray,
                                         const std::string &fun) {
      set1DFunction(fun);
      nData = ndata;
      isHisto = false;
      xMin = xarray[0];
      xMax = xarray[ndata - 1];
      xBins.resize(ndata);
      xBins.assign(xarray, xarray + ndata);
      xValues = xBins;
      set1DSpectrumEmpty();
      auto space =
          boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace);
      space->dataY(0).assign(yarray, yarray + ndata);
      space->dataE(0).assign(ndata, 1.0);
      outputName = "out";
    }

    void check1DSpectrum() {
      TS_ASSERT(isExecuted);
      setDefaultXRange();
      double sum2 = 0.0;
      double sum2w = 0.0;
      auto &yValues =
          dynamic_cast<MatrixWorkspace &>(*workspace).readY(workspaceIndex);
      auto &eValues =
          dynamic_cast<MatrixWorkspace &>(*workspace).readE(workspaceIndex);
      double dof = -double(nParams);
      for (size_t i = 0; i < xValues.size(); ++i) {
        const double xValue = xValues[i];
        if (xValue >= StartX && xValue <= EndX &&
            isGoodValue(yValues[i], eValues[i])) {
          FunctionDomain1DVector x(xValue);
          FunctionValues y(x);
          function->function(x, y);
          double tmp = yValues[i] - y[0];
          // std::cerr << "test " << xValue << ' ' << yValues[i] << ' ' << y[0]
          // << '\n';
          sum2 += tmp * tmp;
          tmp /= eValues[i];
          sum2w += tmp * tmp;
          dof += 1.0;
        }
      }
      TS_ASSERT_DIFFERS(sum2, 0);
      TS_ASSERT_DELTA(sum2, chiSquared, 1e-10);
      TS_ASSERT_DELTA(sum2w, chiSquaredWeighted, 1e-10);
      if (dof <= 0.0)
        dof = 1.0;
      sum2 /= dof;
      sum2w /= dof;
      TS_ASSERT_DELTA(sum2, chiSquaredDividedByDOF, 1e-10);
      TS_ASSERT_DELTA(sum2w, chiSquaredWeightedDividedByDOF, 1e-10);
    }

    void checkFailed() { TS_ASSERT(!isExecuted); }

    void checkErrors(double *sigma) {
      size_t np = function->nParams();
      TS_ASSERT_EQUALS(np, errors->rowCount());
      for (size_t i = 0; i < np; ++i) {
        TS_ASSERT_LESS_THAN(fabs(errors->Double(i, 6)), 1e-4);
        TS_ASSERT_DELTA(errors->Double(i, 1) / errors->Double(i, 2), 1.0, 1e-5);
        TS_ASSERT_DELTA(0.5 * (errors->Double(i, 4) - errors->Double(i, 3)) /
                            errors->Double(i, 5),
                        1.0, 2.0);
        TS_ASSERT_DELTA(errors->Double(i, 5) / sigma[i], 1.0, 0.01);
      }
    }
  };
};

#endif /* MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_ */
