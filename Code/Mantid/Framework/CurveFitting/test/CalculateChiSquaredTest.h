#ifndef MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_
#define MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CalculateChiSquared.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/EmptyValues.h"

#include <algorithm>

using Mantid::CurveFitting::CalculateChiSquared;
using namespace Mantid;
using namespace Mantid::API;

class CalculateChiSquaredTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateChiSquaredTest *createSuite() {
    return new CalculateChiSquaredTest();
  }
  static void destroySuite(CalculateChiSquaredTest *suite) { delete suite; }

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
    std::cerr << tester.chiSquared << std::endl;
  }

  void test_1D_values_point_data() {
    Tester tester(3,10,false);
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
    TS_ASSERT_DELTA(tester.chiSquared, 153.0, 1.0);
    std::cerr << tester.chiSquared << std::endl;
  }

private:
  class Tester {
  public:
    // input parameters
    size_t nParams;
    size_t nData;
    bool isHisto;
    double xMin;
    double xMax;
    std::vector<double> xBins;
    std::vector<double> xValues;
    std::vector<double> yValues;

    // values for algorithm input properties
    IFunction_sptr function;
    Workspace_sptr workspace;
    int workspaceIndex;
    double StartX;
    double EndX;

    void makeXValues() {
      size_t dlt = isHisto ? 1 : 0;
      xBins.resize(nData + dlt);
      double dx = (xMax - xMin) / double(xBins.size() - 1);
      for (size_t i = 0; i < xBins.size(); ++i) {
        xBins[i] = xMin + i * dx;
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
        if (ix != xBins.end()) EndX = *ix;
        else
          EndX = xMax;
      }
    }

  public:
    // algorithm output
    double chiSquared;

    Tester(size_t np = 3, size_t nd = 10, bool histo = true)
        : nParams(np), nData(nd), isHisto(histo), workspaceIndex(0), xMin(-10),
          xMax(10), StartX(EMPTY_DBL()), EndX(EMPTY_DBL()) {
      makeXValues();
    }

    void runAlgorithm() {
      CalculateChiSquared alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Function", function));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", workspace));
      if (dynamic_cast<IFunction1D *>(function.get())) {
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartX", StartX));
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndX", EndX));
      }
      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());
      chiSquared = alg.getProperty("ChiSquared");
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

    void setWorkspaceIndex() {
      workspaceIndex = 3;
      auto ws = dynamic_cast<MatrixWorkspace*>(workspace.get());
      if (ws)
        yValues = ws->readY(workspaceIndex);
      else
        TS_FAIL("Not a matrix workspace");
    }

    void set1DFunction() {
      const std::string fun =
          "name=UserFunction,Formula=a+b*x+c*x^2,a=1,b=1,c=1";
      function = FunctionFactory::Instance().createInitialized(fun);
      TS_ASSERT_EQUALS(function->nParams(), nParams);
    }

    void set1DSpectrumEmpty() {

      const size_t nSpec = 1;
      size_t dn = isHisto ? 1 : 0;
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec,
                                                       nData + dn, nData);
      space->dataX(0).assign(xBins.begin(), xBins.end());
      workspace = space;
      yValues = space->readY(0);
    }

    void set1DSpectrumValues(const size_t nSpec = 1) {
      size_t dn = isHisto ? 1 : 0;
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec,
                                                       nData + dn, nData);
      space->dataX(0).assign(xBins.begin(), xBins.end());
      for (size_t spec = 0; spec < nSpec; ++spec) {
        for (size_t i = 0; i < nData; ++i) {
          const double x = space->readX(0)[i];
          space->dataY(spec)[i] = (1.1 + 0.1 * double(spec)) * (1.0 + x + x * x);
        }
      }
      workspace = space;
      yValues = space->readY(0);
    }

    void check1DSpectrum() {
      setDefaultXRange();
      double sum2 = 0.0;
      for (size_t i = 0; i < xValues.size(); ++i) {
        const double xValue = xValues[i];
        if (xValue >= StartX && xValue <= EndX) {
          FunctionDomain1DVector x(xValue);
          FunctionValues y(x);
          function->function(x, y);
          const double tmp = yValues[i] - y[0];
          sum2 += tmp * tmp;
        }
      }
      TS_ASSERT_DIFFERS(sum2, 0);
      TS_ASSERT_DELTA(sum2, chiSquared, 1e-10);
    }
  };
};

#endif /* MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_ */
