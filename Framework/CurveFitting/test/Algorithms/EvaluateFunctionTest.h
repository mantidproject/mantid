#ifndef MANTID_CURVEFITTING_EVALUATEFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_EVALUATEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidKernel/EmptyValues.h"

using Mantid::CurveFitting::Algorithms::EvaluateFunction;
using namespace Mantid;
using namespace Mantid::API;

class EvaluateFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EvaluateFunctionTest *createSuite() {
    return new EvaluateFunctionTest();
  }
  static void destroySuite(EvaluateFunctionTest *suite) { delete suite; }

  void test_Init() {
    EvaluateFunction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_1D_histo() {
    Tester1D tester;
    tester.setHistograms();
    tester.runAlgorithm();
    tester.checkResult();
  }

  void test_1D_point_data() {
    Tester1D tester;
    tester.setPointData();
    tester.runAlgorithm();
    tester.checkResult();
  }

  void test_1D_point_data_range() {
    Tester1D tester;
    tester.setPointData();
    tester.setRange();
    tester.runAlgorithm();
    tester.checkResult();
  }

  void test_1D_histo_index() {
    Tester1D tester;
    tester.setHistograms();
    tester.setWorkspaceIndex();
    tester.runAlgorithm();
    tester.checkResult();
  }

  void test_1D_point_data_index() {
    Tester1D tester;
    tester.setPointData();
    tester.setWorkspaceIndex();
    tester.runAlgorithm();
    tester.checkResult();
  }

  void test_1D_point_data_range_index() {
    Tester1D tester;
    tester.setPointData();
    tester.setRange();
    tester.setWorkspaceIndex();
    tester.runAlgorithm();
    tester.checkResult();
  }

  void test_1D_range_outside_workspace_fails() {
    Tester1D tester;
    tester.setRange(0, 30);
    tester.setWorkspaceRange(40, 50);
    tester.setWorkspaceIndex();
    tester.runAlgorithm();
    tester.checkResult(true);
  }

  void test_MD_histo() {
    int nx = 5;
    int ny = 6;
    std::vector<double> signal(nx * ny);
    std::vector<double> extents(4);
    extents[0] = -3;
    extents[1] = 3;
    extents[2] = -3;
    extents[3] = 3;
    std::vector<int> nBins(2);
    nBins[0] = nx;
    nBins[1] = ny;

    auto alg = AlgorithmManager::Instance().create("CreateMDHistoWorkspace");
    alg->initialize();
    alg->setProperty("Dimensionality", 2);
    alg->setProperty("SignalInput", signal);
    alg->setProperty("ErrorInput", signal);
    alg->setProperty("Extents", extents);
    alg->setProperty("NumberOfBins", nBins);
    alg->setProperty("Names", "x,y");
    alg->setProperty("Units", "U,V");
    alg->setProperty("OutputWorkspace", "EvaluateFunction_inWS");
    alg->execute();
    auto inWS =
        AnalysisDataService::Instance().retrieve("EvaluateFunction_inWS");
    TS_ASSERT(inWS);

    alg = AlgorithmManager::Instance().create("EvaluateFunction");
    alg->initialize();
    alg->setPropertyValue("Function",
                          "name=UserFunctionMD,Formula=sin(x)*sin(y)");
    alg->setProperty("InputWorkspace", "EvaluateFunction_inWS");
    alg->setProperty("OutputWorkspace", "EvaluateFunction_outWS");
    alg->execute();
    auto outWS =
        AnalysisDataService::Instance().retrieve("EvaluateFunction_outWS");
    TS_ASSERT(outWS);

    auto mdws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
        "EvaluateFunction_outWS");
    TS_ASSERT(mdws);

    auto iter = mdws->createIterator();
    TS_ASSERT(iter);
    if (!iter)
      return;
    do {
      auto xy = iter->getCenter();
      auto signal = iter->getSignal();
      double value = sin(xy[0]) * sin(xy[1]);
      if (value == 0.0) {
        TS_ASSERT_DELTA(signal, 0.0, 1e-14);
      } else {
        // Precision is lost due to the use of floats in MD workspaces.
        TS_ASSERT_DELTA((signal - value) / value, 0.0, 1e-6);
      }
    } while (iter->next());
  }

  void test_set_workspace_twice() {
    Tester1D tester;
    tester.setHistograms();
    tester.initialiseAndSetWorkspaceTwice();
  }

private:
  class Tester1D {
    // values defining the workspace
    size_t nSpec;
    size_t nData;
    bool isHisto;
    double xMin;
    double xMax;
    std::vector<double> xBins;

    // values for algorithm input properties
    int workspaceIndex;
    double StartX;
    double EndX;
    MatrixWorkspace_sptr workspace;

  public:
    IFunction1D_sptr function;
    std::vector<double> xValues; // values at which the function is evaluated

  private:
    void makeXValues() {
      size_t dlt = isHisto ? 1 : 0;
      xBins.resize(nData + dlt);
      double dx = (xMax - xMin) / double(xBins.size() - 1);
      for (size_t i = 0; i < xBins.size(); ++i) {
        xBins[i] = xMin + double(i) * dx;
      }

      if (workspaceIndex > 0) {
        std::transform(
            xBins.begin(), xBins.end(), xBins.begin(),
            std::bind2nd(std::plus<double>(), double(workspaceIndex)));
      }

      if (isHisto) {
        xValues.resize(nData);
        std::transform(xBins.begin(), xBins.end() - 1, xValues.begin(),
                       std::bind2nd(std::plus<double>(), dx / 2));
      } else {
        xValues = xBins;
      }
    }

    void makeWorkspace() {
      size_t dn = isHisto ? 1 : 0;
      workspace = WorkspaceFactory::Instance().create("Workspace2D", nSpec,
                                                      nData + dn, nData);
      workspace->dataX(workspaceIndex).assign(xBins.begin(), xBins.end());
    }

    void makeFunction() {
      const std::string fun = "name=ExpDecay,Height=50,Lifetime=1";
      function = boost::dynamic_pointer_cast<IFunction1D>(
          FunctionFactory::Instance().createInitialized(fun));
      if (!function) {
        TS_FAIL("A 1D function is expected.");
      }
    }

    void setDefaultXRange() {
      if (StartX == EMPTY_DBL())
        StartX = xBins.front() - 0.001;
      if (EndX == EMPTY_DBL()) {
        EndX = xBins.back() + 0.001;
      }
    }

  public:
    // Outputs
    bool isExecuted;
    MatrixWorkspace_sptr outputWorkspace;

    Tester1D()
        : nSpec(2), nData(100), isHisto(true), xMin(0.0), xMax(30),
          workspaceIndex(0), StartX(EMPTY_DBL()), EndX(EMPTY_DBL()) {}

    void setHistograms() { isHisto = true; }

    void setPointData() { isHisto = false; }

    void setRange() {
      StartX = 2.3;
      EndX = 10;
    }

    void setRange(double newStartX, double newEndX) {
      StartX = newStartX;
      EndX = newEndX;
    }

    void setWorkspaceRange(double newXMin, double newXMax) {
      xMin = newXMin;
      xMax = newXMax;
    }

    void setWorkspaceIndex() { workspaceIndex = 1; }

    void runAlgorithm() {
      makeXValues();
      makeWorkspace();
      makeFunction();

      EvaluateFunction alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(
          "Function", boost::dynamic_pointer_cast<IFunction>(function)));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", workspace));
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("WorkspaceIndex", workspaceIndex));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartX", StartX));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndX", EndX));
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("OutputWorkspace", "EvaluateFunction_outWS"));
      TS_ASSERT_THROWS_NOTHING(alg.execute());

      isExecuted = alg.isExecuted();
      if (isExecuted) {
        outputWorkspace =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                "EvaluateFunction_outWS");
      }
      AnalysisDataService::Instance().clear();
    }

    void checkResult(bool shouldFail = false) {

      if (shouldFail) {
        TS_ASSERT(!isExecuted);
        return;
      }

      TS_ASSERT(isExecuted);
      if (!isExecuted)
        return;
      setDefaultXRange();
      TS_ASSERT_DIFFERS(nData, 0);
      auto &Y = outputWorkspace->readY(1);
      size_t j = 0;
      for (size_t i = 0; i < nData; ++i) {
        if (xValues[i] <= StartX || xValues[i] >= EndX)
          continue;
        FunctionDomain1DVector x(xValues[i]);
        FunctionValues y(x);
        function->function(x, y);
        TS_ASSERT_DIFFERS(y[0], 0.0);
        double tmp = (y[0] - Y[j]) / y[0];
        TS_ASSERT_DELTA(tmp, 0.0, 1e-14);
        ++j;
      }
    }

    void initialiseAndSetWorkspaceTwice() {
      makeXValues();
      makeWorkspace();
      makeFunction();

      EvaluateFunction alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(
          "Function", boost::dynamic_pointer_cast<IFunction>(function)));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", workspace));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", workspace));
    }
  };
};

#endif /* MANTID_CURVEFITTING_EVALUATEFUNCTIONTEST_H_ */
