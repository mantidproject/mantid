// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PDFFourierTransform2.h"
#include <algorithm>
#include <cmath>
#include <numeric>

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid;

// allow testing of protected methods
class PDFFourierTransform2Wrapper : public PDFFourierTransform2 {
public:
  size_t determineMinIndex(double min, const std::vector<double> &X, const std::vector<double> &Y) {
    return PDFFourierTransform2::determineMinIndex(min, X, Y);
  };
  size_t determineMaxIndex(double max, const std::vector<double> &X, const std::vector<double> &Y) {
    return PDFFourierTransform2::determineMaxIndex(max, X, Y);
  };
};

namespace {
/**
 * Create Workspace from 0 to N*dx
 */
Mantid::API::MatrixWorkspace_sptr createWS(size_t n, double dx, const std::string &name, const std::string &unitlabel,
                                           const bool withBadValues = false, const bool makePoints = true) {
  auto nx = (makePoints) ? n : n + 1;
  Mantid::DataObjects::Workspace2D_sptr ws = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, nx, n));

  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

  for (size_t i = 0; i < n; i++) {
    X[i] = double(i) * dx;
    Y[i] = X[i] + 1.0;
    E[i] = sqrt(fabs(X[i]));
    if (!makePoints) {
      X[i] = X[i] - dx / 2; // convert to bin edge
    }
  }
  if (!makePoints) {
    X[n] = X[n - 1] + dx; // final bin edge
  }
  if (withBadValues) {
    Y[0] = std::numeric_limits<double>::quiet_NaN();
    Y[Y.size() - 1] = std::numeric_limits<double>::quiet_NaN();
  }

  ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

  Mantid::API::AnalysisDataService::Instance().add(name, ws);

  return ws;
}
} // namespace

class PDFFourierTransform2Test : public CxxTest::TestSuite {
public:
  void test_Init() {
    PDFFourierTransform2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Execute() {

    API::Workspace_sptr ws = createWS(20, 0.1, "TestInput1", "MomentumTransfer");

    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    TS_ASSERT(pdfft.isExecuted());
  }

  void test_CheckResult() {

    API::Workspace_sptr ws = createWS(20, 0.1, "CheckResult", "MomentumTransfer");

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto &R = pdfws->x(0);
    const auto &GofR = pdfws->y(0);
    const auto &pdfUnit = pdfws->getAxis(0)->unit();

    TS_ASSERT_DELTA(R[0], 0.005, 0.0001);
    TS_ASSERT_DELTA(R[249], 2.495, 0.0001);
    TS_ASSERT_DELTA(GofR[0], 0.01150, 0.0001);
    TS_ASSERT_DELTA(GofR[249], -0.6148, 0.0001);
    TS_ASSERT_EQUALS(pdfUnit->caption(), "Atomic Distance");
  }

  // Tests that the algorithm with execute for each of the different PDFTypes
  void test_check_PDFType_executes() {

    std::string PDFTypeArray[4] = {"g(r)", "G(r)", "RDF(r)", "G_k(r)"};

    API::Workspace_sptr ws = createWS(20, 0.1, "TestInput2", "MomentumTransfer");

    // create the material as this is required for G_k(r)
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;

    auto const rho0 = 0.07192;

    auto material = std::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", "V"), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleNumberDensity", rho0), "");

    // set the sample information
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", ws);
    setsample.setProperty("Material", material);
    setsample.execute();

    for (std::string value : PDFTypeArray) {
      PDFFourierTransform2 pdfft;
      pdfft.initialize();
      pdfft.setProperty("InputWorkspace", ws);
      pdfft.setProperty("Direction", "Forward");
      pdfft.setProperty("OutputWorkspace", "PDFGofR");
      pdfft.setProperty("SofQType", "S(Q)");
      pdfft.setProperty("Rmax", 20.0);
      pdfft.setProperty("DeltaR", 0.01);
      pdfft.setProperty("Qmin", 0.0);
      pdfft.setProperty("Qmax", 30.0);
      pdfft.setProperty("PDFType", value);

      TS_ASSERT_THROWS_NOTHING(pdfft.execute());
      TS_ASSERT(pdfft.isExecuted());
    }
  }

  void test_CheckNan() {

    API::Workspace_sptr ws = createWS(20, 0.1, "CheckNan", "MomentumTransfer", true);

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto &R = pdfws->x(0);
    const auto &GofR = pdfws->y(0);

    TS_ASSERT_DELTA(R[0], 0.005, 0.0001);
    TS_ASSERT_DELTA(R[249], 2.495, 0.0001);
    // make sure that nan didn' slip in
    TS_ASSERT(std::find_if(GofR.begin(), GofR.end(), [](const double d) { return std::isnan(d); }) == GofR.end());
  }

  void test_filter() {
    API::MatrixWorkspace_sptr ws = createWS(200, 0.1, "filter", "MomentumTransfer");
    auto &SofQ = ws->mutableY(0);
    for (size_t i = 0; i < SofQ.size(); i++) {
      SofQ[i] = 1.0;
    }

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)-1");
    pdfft.setProperty("Qmax", 20.0);
    pdfft.setProperty("PDFType", "G(r)");
    pdfft.setProperty("Filter", true);

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto &GofR = pdfws->y(0);

    TS_ASSERT(GofR[0] > 10.0);
    for (size_t i = 1; i < GofR.size(); i++) {
      TS_ASSERT_LESS_THAN(fabs(GofR[i]), 0.2);
    }

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_reverse() {
    API::Workspace_sptr ws = createWS(20, 0.1, "CheckResult", "AtomicDistance");

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Backward");
    pdfft.setProperty("OutputWorkspace", "SofQ");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Qmax", 20.0);
    pdfft.setProperty("DeltaQ", 0.01);
    pdfft.setProperty("Rmin", 0.0);
    pdfft.setProperty("Rmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr sofqws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("SofQ"));
    const auto &Q = sofqws->x(0);
    const auto &SofQ = sofqws->y(0);
    const auto &SofQUnit = sofqws->getAxis(0)->unit();

    TS_ASSERT_DELTA(Q[0], 0.005, 0.0001);
    TS_ASSERT_DELTA(Q[249], 2.495, 0.0001);
    TS_ASSERT_DELTA(SofQ[0], 5.58335, 0.0001);
    TS_ASSERT_DELTA(SofQ[249], 1.0678, 0.0001);
    TS_ASSERT_EQUALS(SofQUnit->caption(), "q");
  }

  void test_integration_range() {
    std::vector<double> X;
    std::vector<double> Y;

    for (size_t i = 0; i < 100; i++) {
      X.push_back(double(i) * 0.1);
      Y.push_back(X[i] + 1.0);
    }
    X.push_back(double(100) * 0.1);
    // For a distribution workspace, X.size() = Y.size() + 1
    // This data has 100 bins and 101 bin edges

    std::vector<double> badValuesY = Y;
    badValuesY[0] = std::numeric_limits<double>::quiet_NaN();
    badValuesY[badValuesY.size() - 1] = std::numeric_limits<double>::quiet_NaN();
    PDFFourierTransform2Wrapper alg;
    TS_ASSERT_EQUALS(alg.determineMinIndex(0.0, X, Y), 0);
    TS_ASSERT_EQUALS(alg.determineMinIndex(1.0, X, Y), 10);
    TS_ASSERT_EQUALS(alg.determineMinIndex(0.0, X, badValuesY), 1);
    TS_ASSERT_EQUALS(alg.determineMinIndex(1.0, X, badValuesY), 10);

    TS_ASSERT_EQUALS(alg.determineMaxIndex(5.0, X, Y), 50);
    TS_ASSERT_EQUALS(alg.determineMaxIndex(20.0, X, Y), 100);
    TS_ASSERT_EQUALS(alg.determineMaxIndex(5.0, X, badValuesY), 50);
    TS_ASSERT_EQUALS(alg.determineMaxIndex(20.0, X, badValuesY), 99);

    std::vector<double> endZeroValuesY = Y;
    endZeroValuesY[0] = 0.0;
    endZeroValuesY[1] = 0.0;
    endZeroValuesY[2] = 0.0;
    endZeroValuesY[97] = 0.0;
    endZeroValuesY[98] = 0.0;
    endZeroValuesY[99] = 0.0;
    TS_ASSERT_EQUALS(alg.determineMinIndex(0.0, X, endZeroValuesY), 3);
    TS_ASSERT_EQUALS(alg.determineMaxIndex(20.0, X, endZeroValuesY), 97);
  }

  void test_PDFTypes_fwd() {
    API::MatrixWorkspace_sptr ws = createWS(20, 0.1, "CheckResult2", "MomentumTransfer");

    auto const rho0 = 0.07192;

    // create the material
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;

    auto material = std::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", "V"), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleNumberDensity", rho0), "");

    // set the sample information
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", ws);
    setsample.setProperty("Material", material);
    setsample.execute();

    // check g(r) returns correct value
    DataObjects::Workspace2D_sptr pdfws_gr = run_pdfft2_alg(ws, "g(r)", "Forward");
    const auto little_gofR = pdfws_gr->y(0);
    const auto gofR_comparison = 3.5310290237;
    TS_ASSERT_DELTA(little_gofR[10], gofR_comparison, 1e-8);

    // check G(r) returns correct value
    DataObjects::Workspace2D_sptr pdfws_big_gr = run_pdfft2_alg(ws, "G(r)", "Forward");
    const auto big_gofR = pdfws_big_gr->y(0);
    const auto R_G = pdfws_big_gr->x(0);
    const auto calculated_big_gofR = (gofR_comparison - 1) * 4. * M_PI * rho0 * R_G[10];
    TS_ASSERT_DELTA(big_gofR[10], calculated_big_gofR, 1e-8);

    // check RDF(r) returns correct value
    DataObjects::Workspace2D_sptr pdfws_rdf_r = run_pdfft2_alg(ws, "RDF(r)", "Forward");
    const auto rdfofR = pdfws_rdf_r->y(0);
    const auto R_RDF = pdfws_rdf_r->x(0);
    const auto calculated_rdfofR = gofR_comparison * 4. * M_PI * rho0 * R_RDF[10] * R_RDF[10];

    TS_ASSERT_DELTA(rdfofR[10], calculated_rdfofR, 1e-8);

    //// check G_k(r) returns correct value
    DataObjects::Workspace2D_sptr pdfws_gkr = run_pdfft2_alg(ws, "G_k(r)", "Forward");
    const auto gkofR = pdfws_gkr->y(0);
    const Kernel::Material &material2 = pdfws_gkr->sample().getMaterial();
    const auto factor = 0.01 * pow(material2.cohScatterLength(), 2);
    const auto calculated_gkofR = (gofR_comparison - 1) * factor;

    TS_ASSERT_DELTA(gkofR[10], calculated_gkofR, 1e-8);
  }

  void test_PDFTypes_bkwd() {
    API::MatrixWorkspace_sptr ws = createWS(20, 0.1, "CheckResult3", "AtomicDistance");

    // shared values for tests
    const double single_x = 2.0;
    std::vector<double> x(2, single_x);
    std::vector<double> dy(2, 0.0);
    std::vector<double> dx(2, 0.0);
    const double rho0 = 1.0;
    const double cohScatLen = 1.0;
    const double factor1 = 4. * M_PI * rho0;

    // set up initial y values
    std::vector<double> y_initial(2, 5.0);

    // Algorithm destructor crashes without workspace properties initialised
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("OutputWorkspace", "outputWS");

    // test g(r)
    const std::string LITTLE_G_OF_R("g(r)");
    auto y = y_initial;
    pdfft.convertToLittleGRMinus1(y, x, dy, dx, LITTLE_G_OF_R, rho0, cohScatLen);
    const auto exp_from_gr = 4.0;
    const auto actual_from_gr = y[0];
    TS_ASSERT_DELTA(actual_from_gr, exp_from_gr, 1e-8);

    const std::string BIG_G_OF_R("G(r)");
    y = y_initial;
    pdfft.convertToLittleGRMinus1(y, x, dy, dx, BIG_G_OF_R, rho0, cohScatLen);
    const auto exp_from_big_gr = y_initial[0] / (factor1 * single_x);
    const auto actual_from_big_gr = y[0];
    TS_ASSERT_DELTA(actual_from_big_gr, exp_from_big_gr, 1e-8);

    const std::string RDF_OF_R("RDF(r)");
    y = y_initial;
    pdfft.convertToLittleGRMinus1(y, x, dy, dx, RDF_OF_R, rho0, cohScatLen);
    const auto exp_from_rdf_r = y_initial[0] / (factor1 * single_x * single_x) - 1.0;
    const auto actual_from_rdf_r = y[0];
    TS_ASSERT_DELTA(actual_from_rdf_r, exp_from_rdf_r, 1e-8);

    const std::string G_K_OF_R("G_k(r)");
    y = y_initial;
    pdfft.convertToLittleGRMinus1(y, x, dy, dx, G_K_OF_R, rho0, cohScatLen);
    const double factor2 = 0.01 * pow(cohScatLen, 2);
    const auto exp_from_gkr = y_initial[0] / factor2;
    const auto actual_from_gkr = y[0];
    TS_ASSERT_DELTA(actual_from_gkr, exp_from_gkr, 1e-8);
  }

  void test_points_and_hist_input_give_same_answer() {
    API::MatrixWorkspace_sptr wsPoints = createWS(20, 0.1, "CheckResultPoints", "MomentumTransfer", false, true);
    API::MatrixWorkspace_sptr wsHist = createWS(20, 0.1, "CheckResultHist", "MomentumTransfer", false, false);

    // check g(r) returns correct value
    DataObjects::Workspace2D_sptr grPoints = run_pdfft2_alg(wsPoints, "g(r)", "Forward");
    DataObjects::Workspace2D_sptr grHist = run_pdfft2_alg(wsHist, "g(r)", "Forward");

    const auto grPointsY = grPoints->y(0);
    const auto grHistY = grHist->y(0);
    for (size_t ibin = 1; ibin < grPointsY.size(); ibin++) {
      TS_ASSERT_DELTA(grPointsY[ibin], grHistY[ibin], 1e-8);
    }
  }

  void test_output_range_limit_forwards() {
    API::Workspace_sptr ws = createWS(20, 0.1, "RminLimit", "MomentumTransfer");

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmin", 5.0);
    pdfft.setProperty("Rmax", 15.0);
    pdfft.setProperty("DeltaR", 0.1);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto &R = pdfws->x(0);
    const size_t out_size = R.size();
    const auto &GofR = pdfws->y(0);
    const auto &pdfUnit = pdfws->getAxis(0)->unit();

    TS_ASSERT_EQUALS(out_size, 101);
    TS_ASSERT_DELTA(R.front(), 5.05, 0.0001);
    TS_ASSERT_DELTA(R.back(), 15.05, 0.0001);
    TS_ASSERT_DELTA(GofR.front(), 0.3436, 0.0001);
    TS_ASSERT_DELTA(GofR.back(), 0.0813, 0.0001);
    TS_ASSERT_EQUALS(pdfUnit->caption(), "Atomic Distance");
  }

  void test_output_range_limit_backwards() {
    API::Workspace_sptr ws = createWS(20, 0.1, "QminLimit", "AtomicDistance");

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Backward");
    pdfft.setProperty("OutputWorkspace", "SofQ");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Qmin", 10.0);
    pdfft.setProperty("Qmax", 25.0);
    pdfft.setProperty("DeltaQ", 0.1);
    pdfft.setProperty("Rmin", 0.0);
    pdfft.setProperty("Rmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr sofqws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("SofQ"));
    const auto &Q = sofqws->x(0);
    const size_t out_size = Q.size();
    const auto &SofQ = sofqws->y(0);
    const auto &SofQUnit = sofqws->getAxis(0)->unit();

    TS_ASSERT_EQUALS(out_size, 151);
    TS_ASSERT_DELTA(Q.front(), 10.05, 0.0001);
    TS_ASSERT_DELTA(Q.back(), 25.05, 0.0001);
    TS_ASSERT_DELTA(SofQ.front(), 1.00050, 0.0001);
    TS_ASSERT_DELTA(SofQ.back(), 1.00039, 0.0001);
    TS_ASSERT_EQUALS(SofQUnit->caption(), "q");
  }

private:
  DataObjects::Workspace2D_sptr run_pdfft2_alg(API::MatrixWorkspace_sptr ws, std::string PDFType,
                                               std::string Direction) {
    // 1. Run PDFFT
    const auto outName = ws->getName() + "_outputWS";
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", Direction);
    pdfft.setProperty("OutputWorkspace", outName);
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", PDFType);

    pdfft.execute();

    DataObjects::Workspace2D_sptr output_ws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve(outName));

    return output_ws;
  }
};

class PDFFourierTransform2TestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDFFourierTransform2TestPerformance *createSuite() { return new PDFFourierTransform2TestPerformance(); }

  static void destroySuite(PDFFourierTransform2TestPerformance *suite) { delete suite; }

  void setUp() override {
    ws = createWS(2000000, 0.1, "inputWS", "MomentumTransfer");
    pdfft = std::make_shared<PDFFourierTransform2>();
    pdfft->initialize();
    pdfft->setProperty("InputWorkspace", ws);
    pdfft->setProperty("OutputWorkspace", "outputWS");
    pdfft->setProperty("SofQType", "S(Q)");
    pdfft->setProperty("Rmax", 20.0);
    pdfft->setProperty("DeltaR", 0.01);
    pdfft->setProperty("Qmin", 0.0);
    pdfft->setProperty("Qmax", 30.0);
    pdfft->setProperty("PDFType", "G(r)");
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove("outputWS"); }

  void testPerformanceWS() { pdfft->execute(); }

private:
  Mantid::API::MatrixWorkspace_sptr ws;
  Mantid::API::IAlgorithm_sptr pdfft;
};
