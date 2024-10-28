// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValue.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <algorithm>
#include <sstream>
#include <utility>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;

using WS_type = Mantid::DataObjects::Workspace2D_sptr;
using TWS_type = Mantid::DataObjects::TableWorkspace_sptr;

struct Fun {
  double operator()(double, int i) { return double(i + 1); }
};

class PLOTPEAKBYLOGVALUETEST_Fun : public IFunction1D, public ParamFunction {
public:
  PLOTPEAKBYLOGVALUETEST_Fun() : IFunction1D(), ParamFunction() {
    declareParameter("A");
    declareAttribute("WorkspaceIndex", Attribute(0));
  }
  std::string name() const override { return "PLOTPEAKBYLOGVALUETEST_Fun"; }
  void function1D(double *out, const double *, const size_t nData) const override {
    if (nData == 0)
      return;
    const double a = getParameter("A") + static_cast<double>(getAttribute("WorkspaceIndex").asInt());
    std::fill_n(out, nData, a);
  }
};

DECLARE_FUNCTION(PLOTPEAKBYLOGVALUETEST_Fun)

class PropertyNameIs {
public:
  PropertyNameIs(std::string name) : m_name(std::move(name)) {};

  bool operator()(const Mantid::Kernel::PropertyHistory_sptr &p) { return p->name() == m_name; }

private:
  std::string m_name;
};

class PlotPeak_Expression {
public:
  PlotPeak_Expression(const int i) : m_ws(i) {}
  double operator()(double x, int spec) {
    if (spec == 1) {
      const double a = 1. + 0.1 * m_ws;
      const double b = 0.3 - 0.02 * m_ws;
      const double h = 2. - 0.2 * m_ws;
      const double c = 5. + 0.03 * m_ws;
      const double s = 0.1 + 0.01 * m_ws;
      return a + b * x + h * exp(-0.5 * (x - c) * (x - c) / (s * s));
    }
    return 0.;
  }

private:
  const int m_ws;
};

class PlotPeakByLogValueTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotPeakByLogValueTest *createSuite() { return new PlotPeakByLogValueTest(); }
  static void destroySuite(PlotPeakByLogValueTest *suite) { delete suite; }

  PlotPeakByLogValueTest() { FrameworkManager::Instance(); }

  void testWorkspaceGroup() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex", "1");
    alg.setPropertyValue("LogValue", "var");
    alg.setPropertyValue("Function", "name=LinearBackground,A0=1,A1=0.3;name="
                                     "Gaussian,PeakCentre=5,Height=2,Sigma=0."
                                     "1");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(), 14);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(), 14);
    TS_ASSERT_EQUALS(tnames[0], "var");
    TS_ASSERT_EQUALS(tnames[1], "f0.A0");
    TS_ASSERT_EQUALS(tnames[2], "f0.A0_Err");
    TS_ASSERT_EQUALS(tnames[3], "f0.A1");
    TS_ASSERT_EQUALS(tnames[4], "f0.A1_Err");
    TS_ASSERT_EQUALS(tnames[5], "f1.Height");
    TS_ASSERT_EQUALS(tnames[6], "f1.Height_Err");
    TS_ASSERT_EQUALS(tnames[7], "f1.PeakCentre");
    TS_ASSERT_EQUALS(tnames[8], "f1.PeakCentre_Err");
    TS_ASSERT_EQUALS(tnames[9], "f1.Sigma");
    TS_ASSERT_EQUALS(tnames[10], "f1.Sigma_Err");
    TS_ASSERT_EQUALS(tnames[11], "f1.Integrated Intensity");
    TS_ASSERT_EQUALS(tnames[12], "f1.Integrated Intensity_Err");
    TS_ASSERT_EQUALS(tnames[13], "Chi_squared");

    TS_ASSERT_DELTA(result->Double(0, 0), 1, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 1), 1, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 3), 0.3, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 5), 2, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 7), 5, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 9), 0.1, 1e-10);

    TS_ASSERT_DELTA(result->Double(1, 0), 1.3, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 1), 1.1, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 3), 0.28, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 5), 1.8, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 7), 5.03, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 9), 0.11, 1e-10);

    TS_ASSERT_DELTA(result->Double(2, 0), 1.6, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 1), 1.2, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 3), 0.26, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 5), 1.6, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 7), 5.06, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 9), 0.12, 1e-10);

    /* Check intensity column: */
    TS_ASSERT_DELTA(result->Double(0, 11), 0.501326, 1e-6);
    TS_ASSERT_DELTA(result->Double(1, 11), 0.496312, 1e-6);
    TS_ASSERT_DELTA(result->Double(2, 11), 0.481273, 1e-6);

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");
  }

  void testWorkspaceList() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex", "1");
    alg.setPropertyValue("LogValue", "var");
    alg.setPropertyValue("Function", "name=LinearBackground,A0=1,A1=0.3;name="
                                     "Gaussian,PeakCentre=5,Height=2,Sigma=0."
                                     "1");
    alg.execute();

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(), 14);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(), 14);
    TS_ASSERT_EQUALS(tnames[0], "var");
    TS_ASSERT_EQUALS(tnames[1], "f0.A0");
    TS_ASSERT_EQUALS(tnames[2], "f0.A0_Err");
    TS_ASSERT_EQUALS(tnames[3], "f0.A1");
    TS_ASSERT_EQUALS(tnames[4], "f0.A1_Err");
    TS_ASSERT_EQUALS(tnames[5], "f1.Height");
    TS_ASSERT_EQUALS(tnames[6], "f1.Height_Err");
    TS_ASSERT_EQUALS(tnames[7], "f1.PeakCentre");
    TS_ASSERT_EQUALS(tnames[8], "f1.PeakCentre_Err");
    TS_ASSERT_EQUALS(tnames[9], "f1.Sigma");
    TS_ASSERT_EQUALS(tnames[10], "f1.Sigma_Err");
    TS_ASSERT_EQUALS(tnames[11], "f1.Integrated Intensity");
    TS_ASSERT_EQUALS(tnames[12], "f1.Integrated Intensity_Err");
    TS_ASSERT_EQUALS(tnames[13], "Chi_squared");

    TS_ASSERT_DELTA(result->Double(0, 0), 1, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 1), 1, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 3), 0.3, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 5), 2, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 7), 5, 1e-10);
    TS_ASSERT_DELTA(result->Double(0, 9), 0.1, 1e-10);

    TS_ASSERT_DELTA(result->Double(1, 0), 1.3, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 1), 1.1, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 3), 0.28, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 5), 1.8, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 7), 5.03, 1e-10);
    TS_ASSERT_DELTA(result->Double(1, 9), 0.11, 1e-10);

    TS_ASSERT_DELTA(result->Double(2, 0), 1.6, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 1), 1.2, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 3), 0.26, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 5), 1.6, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 7), 5.06, 1e-10);
    TS_ASSERT_DELTA(result->Double(2, 9), 0.12, 1e-10);

    /* Check intensity column: */
    TS_ASSERT_DELTA(result->Double(0, 11), 0.501326, 1e-6);
    TS_ASSERT_DELTA(result->Double(1, 11), 0.496312, 1e-6);
    TS_ASSERT_DELTA(result->Double(2, 11), 0.481273, 1e-6);

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");
  }

  void testWorkspaceList_plotting_against_ws_names() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex", "1");
    alg.setPropertyValue("LogValue", "SourceName");
    alg.setPropertyValue("Function", "name=LinearBackground,A0=1,A1=0.3;name="
                                     "Gaussian,PeakCentre=5,Height=2,Sigma=0."
                                     "1");
    alg.execute();

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(), 14);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(), 14);
    TS_ASSERT_EQUALS(tnames[0], "SourceName");

    TS_ASSERT_EQUALS(result->String(0, 0), "PlotPeakGroup_0");
    TS_ASSERT_EQUALS(result->String(1, 0), "PlotPeakGroup_1");
    TS_ASSERT_EQUALS(result->String(2, 0), "PlotPeakGroup_2");

    /* Check intensity column: */
    TS_ASSERT_DELTA(result->Double(0, 11), 0.501326, 1e-6);
    TS_ASSERT_DELTA(result->Double(1, 11), 0.496312, 1e-6);
    TS_ASSERT_DELTA(result->Double(2, 11), 0.481273, 1e-6);

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");
  }

  void testSpectraList_plotting_against_bin_edge_axis() {
    auto ws = createTestWorkspace();
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,i0;PLOTPEAKBYLOGVALUETEST_WS,i1");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setPropertyValue("Function", "name=LinearBackground,A0=1,A1=0.3;name="
                                     "Gaussian,PeakCentre=5,Height=2,Sigma=0."
                                     "1");
    alg.execute();

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(), 14);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(), 14);
    TS_ASSERT_EQUALS(tnames[0], "axis-1");

    TS_ASSERT_EQUALS(result->Double(0, 0), 0.5);
    TS_ASSERT_EQUALS(result->Double(1, 0), 3.0);

    WorkspaceCreationHelper::removeWS("PlotPeakResult");
    WorkspaceCreationHelper::removeWS("PLOTPEAKBYLOGVALUETEST_WS");
  }

  void test_passWorkspaceIndexToFunction() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(Fun(), 3, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction", true);
    alg.setPropertyValue("Function", "name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT(result);

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    TableRow row = result->getFirstRow();
    do {
      TS_ASSERT_DELTA(row.Double(1), 1.0, 1e-15);
    } while (row.next());

    AnalysisDataService::Instance().clear();
  }

  void test_dont_passWorkspaceIndexToFunction() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(Fun(), 3, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction", false);
    alg.setPropertyValue("Function", "name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT(result);

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    double a = 1.0;
    TableRow row = result->getFirstRow();
    do {
      TS_ASSERT_DELTA(row.Double(1), a, 1e-15);
      a += 1.0;
    } while (row.next());

    AnalysisDataService::Instance().clear();
  }

  void test_passWorkspaceIndexToFunction_composit_function_case() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(Fun(), 3, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction", true);
    alg.setPropertyValue("Function", "name=FlatBackground,ties=(A0=0.5);name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT(result);

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    TableRow row = result->getFirstRow();
    do {
      TS_ASSERT_DELTA(row.Double(1), 0.5, 1e-15);
    } while (row.next());

    AnalysisDataService::Instance().clear();
  }

  void test_createOutputOption() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(Fun(), 3, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction", true);
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function", "name=FlatBackground,ties=(A0=0.5);name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT(result);

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    TableRow row = result->getFirstRow();
    do {
      TS_ASSERT_DELTA(row.Double(1), 0.5, 1e-15);
    } while (row.next());

    auto matrices =
        AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_NormalisedCovarianceMatrices");
    auto params = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Parameters");
    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");

    TS_ASSERT(matrices);
    TS_ASSERT(params);
    TS_ASSERT(fits);

    TS_ASSERT(matrices->getNames().size() == 3);
    TS_ASSERT(params->getNames().size() == 3);
    TS_ASSERT(fits->getNames().size() == 3);

    AnalysisDataService::Instance().clear();
  }

  void test_createOutputOptionMultipleWorkspaces() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex", "1");
    alg.setPropertyValue("LogValue", "var");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function", "name=LinearBackground,A0=1,A1=0.3;name="
                                     "Gaussian,PeakCentre=5,Height=2,Sigma=0."
                                     "1");
    TS_ASSERT(alg.execute());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(), 14);

    auto matrices =
        AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_NormalisedCovarianceMatrices");
    auto params = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Parameters");
    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");

    TS_ASSERT(matrices);
    TS_ASSERT(params);
    TS_ASSERT(fits);

    TS_ASSERT(matrices->getNames().size() == 3);
    TS_ASSERT(params->getNames().size() == 3);
    TS_ASSERT(fits->getNames().size() == 3);
  }

  void test_createOutputWithExtraOutputOptions() {
    auto ws = createTestWorkspace();
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v0:2");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction", true);
    alg.setProperty("CreateOutput", true);
    alg.setProperty("OutputCompositeMembers", true);
    alg.setProperty("ConvolveMembers", true);
    alg.setPropertyValue("Function", "name=LinearBackground,A0=0,A1=0;"
                                     "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                     "name=Resolution,Workspace=PLOTPEAKBYLOGVALUETEST_WS,WorkspaceIndex=0;"
                                     "name=Gaussian,Height=3000,PeakCentre=6493,Sigma=50;);");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT(result);

    auto matrices =
        AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_NormalisedCovarianceMatrices");
    auto params = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Parameters");
    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");

    TS_ASSERT(matrices);
    TS_ASSERT(params);
    TS_ASSERT(fits);

    TS_ASSERT(matrices->getNames().size() == 2);
    TS_ASSERT(params->getNames().size() == 2);
    TS_ASSERT(fits->getNames().size() == 2);

    auto wsNames = fits->getNames();
    for (const auto &wsName : wsNames) {
      auto fit = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName);
      TS_ASSERT(fit);
      TS_ASSERT(fit->getNumberHistograms() == 5);
    }

    AnalysisDataService::Instance().clear();
  }

  void testMinimizer() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("WorkspaceIndex", "1");
    alg.setPropertyValue("Function", "name=LinearBackground,A0=1,A1=0.3;name="
                                     "Gaussian,PeakCentre=5,Height=2,Sigma=0."
                                     "1");
    alg.setPropertyValue("MaxIterations", "50");
    // This is a stupid use case but will at least demonstrate the functionality
    alg.setPropertyValue("Minimizer", "Levenberg-Marquardt,AbsError=0.01,RelError=$wsindex");

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");
    TS_ASSERT(fits);

    if (fits->size() > 0) {
      auto fit = fits->getItem(0);
      TS_ASSERT_EQUALS(fit->history().size(), 0);
      TS_ASSERT_EQUALS(fit->getName(), "PlotPeakResult_Workspaces_1");
    }

    AnalysisDataService::Instance().clear();
  }

  void test_parameters_are_correct_for_a_histogram_fit() {
    createHistogramWorkspace("InputWS", 10, -10.0, 10.0);

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setAlwaysStoreInADS(false);
    alg.setProperty("EvaluationType", "Histogram");
    alg.setPropertyValue("Input", "InputWS,v1:3");
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function", "name=FlatBackground,A0=2");
    alg.execute();

    ITableWorkspace_sptr params = alg.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(params->Double(0, 1), 1.0, 1e-15);
    TS_ASSERT_DELTA(params->Double(1, 1), 1.1, 1e-15);
    TS_ASSERT_DELTA(params->Double(2, 1), 0.6, 1e-15);

    AnalysisDataService::Instance().clear();
  }

  void test_single_exclude_range_single_Spectra() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0");
    alg.setPropertyValue("Exclude", "-0.5, 0.5");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function", "name=FlatBackground,A0=2");
    alg.setPropertyValue("MaxIterations", "50");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");
  }

  void test_single_exclude_range_multiple_Spectra() {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0;PlotPeakGroup_1");
    alg.setPropertyValue("Exclude", "-0.5, 0.5");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function", "name=FlatBackground,A0=2");
    alg.setPropertyValue("MaxIterations", "50");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");
  }

  void test_multiple_exclude_range_multiple_Spectra() {
    createData();
    std::vector<std::string> excludeRanges;
    excludeRanges.emplace_back("-0.5, 0.0");
    excludeRanges.emplace_back("0.5, 1.5");
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PlotPeakGroup_0;PlotPeakGroup_1");
    alg.setProperty("ExcludeMultiple", excludeRanges);
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function", "name=FlatBackground,A0=2");
    alg.setPropertyValue("MaxIterations", "50");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");
  }

  void test_startX_single_value() {
    auto ws = createTestWorkspace();
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v0:2");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("StartX", "1000.0");
    alg.setProperty("EndX", "3000.0");
    alg.setProperty("PassWSIndexToFunction", true);
    alg.setProperty("CreateOutput", true);
    alg.setProperty("OutputCompositeMembers", true);
    alg.setProperty("ConvolveMembers", true);
    alg.setPropertyValue("Function", "name=LinearBackground,A0=0,A1=0;"
                                     "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                     "name=Resolution,Workspace=PLOTPEAKBYLOGVALUETEST_WS,WorkspaceIndex=0;"
                                     "name=Gaussian,Height=3000,PeakCentre=6493,Sigma=50;);");
    alg.execute();

    TS_ASSERT(alg.isExecuted());
    AnalysisDataService::Instance().remove("PLOTPEAKBYLOGVALUETEST_WS");
  }

  void test_startX_multiple_value() {
    auto ws = createTestWorkspace();
    AnalysisDataService::Instance().add("PLOTPEAKBYLOGVALUETEST_WS", ws);
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input", "PLOTPEAKBYLOGVALUETEST_WS,v0:2");
    alg.setPropertyValue("OutputWorkspace", "PlotPeakResult");
    alg.setProperty("StartX", "1000.0,1000.0");
    alg.setProperty("EndX", "3000.0,3000.0");
    alg.setProperty("PassWSIndexToFunction", true);
    alg.setProperty("CreateOutput", true);
    alg.setProperty("OutputCompositeMembers", true);
    alg.setProperty("ConvolveMembers", true);
    alg.setPropertyValue("Function", "name=LinearBackground,A0=0,A1=0;"
                                     "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                     "name=Resolution,Workspace=PLOTPEAKBYLOGVALUETEST_WS,WorkspaceIndex=0;"
                                     "name=Gaussian,Height=3000,PeakCentre=6493,Sigma=50;);");
    alg.execute();

    TS_ASSERT(alg.isExecuted());
    AnalysisDataService::Instance().remove("PLOTPEAKBYLOGVALUETEST_WS");
  }

private:
  WorkspaceGroup_sptr m_wsg;

  void createData(bool hist = false) {
    m_wsg.reset(new WorkspaceGroup);
    AnalysisDataService::Instance().add("PlotPeakGroup", m_wsg);
    const int N = 3;
    for (int iWS = 0; iWS < N; ++iWS) {
      auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(PlotPeak_Expression(iWS), 3, 0, 10, 0.005, hist);
      for (int i = 0; i < 3; ++i) {
        ws->getSpectrum(i).setSpectrumNo(0);
      }
      Kernel::TimeSeriesProperty<double> *logd = new Kernel::TimeSeriesProperty<double>("var");
      logd->addValue("2007-11-01T18:18:53", 1 + iWS * 0.3);
      ws->mutableRun().addLogData(logd);
      std::ostringstream wsName;
      wsName << "PlotPeakGroup_" << iWS;
      WorkspaceCreationHelper::storeWS(wsName.str(), ws);
      m_wsg->add(wsName.str());
    }
  }

  void createHistogramWorkspace(const std::string &name, std::size_t nbins, double x0, double x1) {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 3, nbins + 1, nbins);
    double dx = (x1 - x0) / static_cast<double>(nbins);
    ws->setBinEdges(0, nbins + 1, HistogramData::LinearGenerator(x0, dx));
    ws->setSharedX(1, ws->sharedX(0));
    ws->setSharedX(2, ws->sharedX(0));

    std::vector<double> fwhms{1.0, 1.1, 0.6};
    for (size_t i = 0; i < 3; ++i) {
      std::string fun = "name=FlatBackground,A0=" + std::to_string(fwhms[i]);
      auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
      alg->initialize();
      alg->setProperty("EvaluationType", "Histogram");
      alg->setProperty("Function", fun);
      alg->setProperty("InputWorkspace", ws);
      alg->setProperty("OutputWorkspace", "out");
      alg->execute();
      auto calc = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
      ws->dataY(i) = calc->readY(1);
    }
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }

  MatrixWorkspace_sptr createTestWorkspace() {
    const int numHists(2);
    const int numBins(2000);
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numHists, numBins, true);
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    // Update X data  to a sensible values. Looks roughly like the MARI binning
    BinEdges xdata(numBins + 1, LinearGenerator(5.0, 5.5));
    // Update the Y values. We don't care about errors here

    // We'll simply use a gaussian as a test
    const double peakOneCentre(6493.0), sigmaSqOne(250 * 250.), peakTwoCentre(10625.), sigmaSqTwo(50 * 50);
    const double peakOneHeight(3000.), peakTwoHeight(1000.);
    for (int i = 0; i < numBins; ++i) {
      testWS->dataY(0)[i] = peakOneHeight * exp(-0.5 * pow(xdata[i] - peakOneCentre, 2.) / sigmaSqOne);
      testWS->dataY(1)[i] = peakTwoHeight * exp(-0.5 * pow(xdata[i] - peakTwoCentre, 2.) / sigmaSqTwo);
    }
    testWS->setBinEdges(0, xdata);
    testWS->setBinEdges(1, xdata);

    std::vector<double> edges;
    edges.emplace_back(0.0);
    edges.emplace_back(1.0);
    edges.emplace_back(5.0);
    auto axis = std::make_unique<BinEdgeAxis>(edges);
    testWS->replaceAxis(1, std::move(axis));

    return testWS;
  }

  void deleteData() {
    FrameworkManager::Instance().deleteWorkspace(m_wsg->getName());
    m_wsg.reset();
  }
};
