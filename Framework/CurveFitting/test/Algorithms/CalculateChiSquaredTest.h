// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/CalculateChiSquared.h"
#include "MantidKernel/EmptyValues.h"

#include <algorithm>
#include <cmath>
#include <limits>

using Mantid::CurveFitting::Algorithms::CalculateChiSquared;
using namespace Mantid;
using namespace Mantid::API;

class CalculateChiSquaredTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateChiSquaredTest *createSuite() { return new CalculateChiSquaredTest(); }
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

    void makeXValues() {
      size_t dlt = isHisto ? 1 : 0;
      xBins.resize(nData + dlt);
      double dx = (xMax - xMin) / double(xBins.size() - 1);
      for (size_t i = 0; i < xBins.size(); ++i) {
        xBins[i] = xMin + double(i) * dx;
      }
      if (isHisto) {
        xValues.resize(nData);
        using std::placeholders::_1;
        std::transform(xBins.begin(), xBins.end() - 1, xValues.begin(), std::bind(std::plus<double>(), _1, dx / 2));
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
      return !ignoreInvalidData || (std::isfinite(y) && std::isfinite(e) && e > 0);
    }

  public:
    // algorithm output
    double chiSquared;
    double chiSquaredDividedByDOF;
    double chiSquaredWeighted;
    double chiSquaredWeightedDividedByDOF;
    bool isExecuted;

    Tester(size_t np = 3, size_t nd = 10, bool histo = true)
        : nParams(np), nData(nd), isHisto(histo), xMin(-10), xMax(10), workspaceIndex(0), StartX(EMPTY_DBL()),
          EndX(EMPTY_DBL()), ignoreInvalidData(false),
          // output
          chiSquared(-1), chiSquaredDividedByDOF(-1), chiSquaredWeighted(-1), chiSquaredWeightedDividedByDOF(-1),
          isExecuted(false) {
      makeXValues();
    }

    void runAlgorithm() {
      CalculateChiSquared alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Function", function));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", workspace));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("IgnoreInvalidData", ignoreInvalidData));
      if (dynamic_cast<IFunction1D *>(function.get())) {
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndex", workspaceIndex));
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartX", StartX));
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndX", EndX));
      }
      TS_ASSERT_THROWS_NOTHING(alg.execute());
      isExecuted = alg.isExecuted();
      if (isExecuted) {
        chiSquared = alg.getProperty("ChiSquared");
        chiSquaredDividedByDOF = alg.getProperty("ChiSquaredDividedByDOF");
        chiSquaredWeighted = alg.getProperty("ChiSquaredWeighted");
        chiSquaredWeightedDividedByDOF = alg.getProperty("ChiSquaredWeightedDividedByDOF");
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

    void set1DFunction(const std::string &fun = "name=UserFunction,Formula=a+b*x+c*x^2,a=1,b=1,c=1") {
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
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec, nData + dn, nData);
      space->dataX(0).assign(xBins.begin(), xBins.end());
      space->dataE(0).assign(nData, 10.0);
      workspace = space;
    }

    void set1DSpectrumValues(const size_t nSpec = 1) {
      size_t dn = isHisto ? 1 : 0;
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec, nData + dn, nData);
      for (size_t spec = 0; spec < nSpec; ++spec) {
        space->dataX(spec).assign(xBins.begin(), xBins.end());
        for (size_t i = 0; i < nData; ++i) {
          const double x = space->readX(0)[i];
          space->dataY(spec)[i] = (1.1 + 0.1 * double(spec)) * (1.0 + x + x * x);
          space->dataE(spec)[i] = 10.0;
        }
      }
      workspace = space;
    }

    void set1DSpectrumValuesInvalid() {
      set1DSpectrumValues();
      auto &yValues = dynamic_cast<MatrixWorkspace &>(*workspace).dataY(workspaceIndex);
      yValues[2] = std::numeric_limits<double>::infinity();
      yValues[4] = std::numeric_limits<double>::quiet_NaN();
      auto &eValues = dynamic_cast<MatrixWorkspace &>(*workspace).dataE(workspaceIndex);
      eValues[6] = -1;
    }

    void setTestCaseForErrorCalculations(size_t ndata, double *xarray, double *yarray, const std::string &fun) {
      set1DFunction(fun);
      nData = ndata;
      isHisto = false;
      xMin = xarray[0];
      xMax = xarray[ndata - 1];
      xBins.resize(ndata);
      xBins.assign(xarray, xarray + ndata);
      xValues = xBins;
      set1DSpectrumEmpty();
      auto space = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace);
      space->dataY(0).assign(yarray, yarray + ndata);
      space->dataE(0).assign(ndata, 1.0);
      outputName = "out";
    }

    void check1DSpectrum() {
      TS_ASSERT(isExecuted);
      setDefaultXRange();
      double sum2 = 0.0;
      double sum2w = 0.0;
      auto &yValues = dynamic_cast<MatrixWorkspace &>(*workspace).readY(workspaceIndex);
      auto &eValues = dynamic_cast<MatrixWorkspace &>(*workspace).readE(workspaceIndex);
      double dof = -double(nParams);
      for (size_t i = 0; i < xValues.size(); ++i) {
        const double xValue = xValues[i];
        if (xValue >= StartX && xValue <= EndX && isGoodValue(yValues[i], eValues[i])) {
          FunctionDomain1DVector x(xValue);
          FunctionValues y(x);
          function->function(x, y);
          double tmp = yValues[i] - y[0];
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
  };
};
