#ifndef CURVEFITTING_FITMWTEST_H_
#define CURVEFITTING_FITMWTEST_H_

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Polynomial.h"
#include "MantidCurveFitting/Functions/UserFunction.h"
#include "MantidCurveFitting/SeqDomain.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/PropertyManager.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

namespace {
API::MatrixWorkspace_sptr createTestWorkspace(const bool histogram,
                                              size_t NVectors = 2,
                                              size_t YLength = 20) {
  MatrixWorkspace_sptr ws2(new WorkspaceTester);
  size_t XLength = YLength + (histogram ? 1 : 0);
  ws2->initialize(NVectors, XLength, YLength);

  for (size_t is = 0; is < ws2->getNumberHistograms(); ++is) {
    auto &x = ws2->mutableX(is);
    auto &y = ws2->mutableY(is);
    for (size_t i = 0; i < y.size(); ++i) {
      x[i] = 0.1 * static_cast<double>(i);
      const double is_d = static_cast<double>(is);
      y[i] = (10.0 + is_d) * exp(-(x[i]) / (0.5 * (1. + is_d)));
    }
    if (histogram)
      x.back() = x[x.size() - 2] + 0.1;
  }
  return ws2;
}

void doTestExecPointData(API::MatrixWorkspace_sptr ws2,
                         bool performance = false) {
  API::IFunction_sptr fun(new ExpDecay);
  fun->setParameter("Height", 1.);
  fun->setParameter("Lifetime", 1.0);

  Algorithms::Fit fit;
  fit.initialize();

  fit.setProperty("Function", fun);
  fit.setProperty("InputWorkspace", ws2);
  fit.setProperty("WorkspaceIndex", 0);
  fit.setProperty("CreateOutput", true);

  fit.execute();
  if (!performance) {
    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-8);
    // TS_ASSERT_DIFFERS(chi2, 0.0);
    TS_ASSERT_EQUALS(fit.getPropertyValue("OutputStatus"), "success");

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::AnalysisDataService::Instance().retrieve("Output_Workspace"));
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 3);
    API::Axis *axis = outWS->getAxis(1);
    TS_ASSERT(axis);
    TS_ASSERT(axis->isText());
    TS_ASSERT_EQUALS(axis->length(), 3);
    TS_ASSERT_EQUALS(axis->label(0), "Data");
    TS_ASSERT_EQUALS(axis->label(1), "Calc");
    TS_ASSERT_EQUALS(axis->label(2), "Diff");

    const auto &Data = outWS->y(0);
    const auto &Calc = outWS->y(1);
    const auto &Diff = outWS->y(2);
    for (size_t i = 0; i < Data.size(); ++i) {
      TS_ASSERT_EQUALS(Data[i] - Calc[i], Diff[i]);
    }

    ITableWorkspace_sptr covar = boost::dynamic_pointer_cast<ITableWorkspace>(
        API::AnalysisDataService::Instance().retrieve(
            "Output_NormalisedCovarianceMatrix"));

    TS_ASSERT(covar);
    TS_ASSERT_EQUALS(covar->columnCount(), 3);
    TS_ASSERT_EQUALS(covar->rowCount(), 2);
    TS_ASSERT_EQUALS(covar->String(0, 0), "Height");
    TS_ASSERT_EQUALS(covar->String(1, 0), "Lifetime");
    TS_ASSERT_EQUALS(covar->getColumn(0)->type(), "str");
    TS_ASSERT_EQUALS(covar->getColumn(0)->name(), "Name");
    TS_ASSERT_EQUALS(covar->getColumn(1)->type(), "double");
    TS_ASSERT_EQUALS(covar->getColumn(1)->name(), "Height");
    TS_ASSERT_EQUALS(covar->getColumn(2)->type(), "double");
    TS_ASSERT_EQUALS(covar->getColumn(2)->name(), "Lifetime");
    TS_ASSERT_EQUALS(covar->Double(0, 1), 100.0);
    TS_ASSERT_EQUALS(covar->Double(1, 2), 100.0);
    TS_ASSERT(fabs(covar->Double(0, 2)) < 100.0);
    TS_ASSERT(fabs(covar->Double(0, 2)) > 0.0);
    TS_ASSERT_DELTA(covar->Double(0, 2), covar->Double(1, 1), 0.000001);

    TS_ASSERT_DIFFERS(fun->getError(0), 0.0);
    TS_ASSERT_DIFFERS(fun->getError(1), 0.0);

    ITableWorkspace_sptr params = boost::dynamic_pointer_cast<ITableWorkspace>(
        API::AnalysisDataService::Instance().retrieve("Output_Parameters"));

    TS_ASSERT(params);
    TS_ASSERT_EQUALS(params->columnCount(), 3);
    TS_ASSERT_EQUALS(params->rowCount(), 3);
    TS_ASSERT_EQUALS(params->String(0, 0), "Height");
    TS_ASSERT_EQUALS(params->String(1, 0), "Lifetime");
    TS_ASSERT_EQUALS(params->String(2, 0), "Cost function value");
    TS_ASSERT_EQUALS(params->Double(0, 1), fun->getParameter(0));
    TS_ASSERT_EQUALS(params->Double(1, 1), fun->getParameter(1));
    TS_ASSERT_EQUALS(params->Double(2, 1), chi2);
    TS_ASSERT_EQUALS(params->Double(0, 2), fun->getError(0));
    TS_ASSERT_EQUALS(params->Double(1, 2), fun->getError(1));
    TS_ASSERT_EQUALS(params->Double(2, 2), 0.0);
  }
  API::AnalysisDataService::Instance().clear();
  //--------------------------------------------------//
  if (!performance) {
    // Fit fit1;
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("Fit");
    Mantid::API::IAlgorithm &fit1 = *alg;
    fit1.initialize();

    fit1.setProperty("Function", fun);
    fit1.setProperty("InputWorkspace", ws2);
    fit1.setProperty("WorkspaceIndex", 1);

    fit1.execute();

    TS_ASSERT(fit1.isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 11.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 1.0, 1e-4);
  }
}
void doTestExecHistogramData(API::MatrixWorkspace_sptr ws2,
                             bool performance = false) {
  API::IFunction_sptr fun(new ExpDecay);
  fun->setParameter("Height", 1.);
  fun->setParameter("Lifetime", 1.);

  Algorithms::Fit fit;
  fit.initialize();

  fit.setProperty("Function", fun);
  fit.setProperty("InputWorkspace", ws2);
  fit.setProperty("WorkspaceIndex", 0);

  fit.execute();

  if (!performance) {
    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 11.0517, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);
  }

  Fit fit1;
  fit1.initialize();

  fit1.setProperty("Function", fun);
  fit1.setProperty("InputWorkspace", ws2);
  fit1.setProperty("WorkspaceIndex", 1);

  fit1.execute();
  if (!performance) {
    TS_ASSERT(fit1.isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 11.5639, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 1.0, 1e-4);
  }
}
} // namespace

class FitMWTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitMWTest *createSuite() { return new FitMWTest(); }
  static void destroySuite(FitMWTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  FitMWTest() {
    // need to have DataObjects loaded
    FrameworkManager::Instance();
  }

  void test_exec_point_data() {
    const bool histogram = false;
    doTestExecPointData(createTestWorkspace(histogram));
  }

  void test_exec_histogram_data() {
    const bool histogram = true;
    doTestExecHistogramData(createTestWorkspace(histogram));
  }

  void test_exec_distribution_data_produces_distribution_data() {
    // Arrange
    const bool histogram = true;
    auto ws3 = createTestWorkspace(histogram);
    API::WorkspaceHelpers::makeDistribution(ws3);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 1.);
    fun->setParameter("Lifetime", 1.);

    // Act
    Fit fit;
    fit.initialize();

    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws3);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("CreateOutput", true);
    fit.setProperty("Output", "out");

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    // Assert
    Mantid::API::MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("out_Workspace");
    TS_ASSERT(out_ws);
    TSM_ASSERT("Output should be a distribution", out_ws->isDistribution());
    API::AnalysisDataService::Instance().clear();
  }

  // test that errors of the calculated output are reasonable
  void test_output_errors() {
    const bool histogram = true;
    auto ws2 = createTestWorkspace(histogram);

    API::IFunction_sptr fun(new Polynomial);
    fun->setAttributeValue("n", 5);

    Fit fit;
    fit.initialize();

    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws2);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("Output", "out");

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    Mantid::API::MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("out_Workspace");
    TS_ASSERT(out_ws);
    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 3);
    auto &e = out_ws->e(1);
    for (size_t i = 0; i < e.size(); ++i) {
      TS_ASSERT(e[i] < 1.0);
    }

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_all_output() {
    auto ws2 = createTestWorkspace(true);

    API::IFunction_sptr fun(new Polynomial);
    fun->setAttributeValue("n", 1);

    Fit fit;
    fit.initialize();

    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws2);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("Output", "out");

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Workspace"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Parameters"));

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_output_parameters_only() {
    auto ws2 = createTestWorkspace(true);

    API::IFunction_sptr fun(new Polynomial);
    fun->setAttributeValue("n", 1);

    Fit fit;
    fit.initialize();

    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws2);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("Output", "out");
    fit.setProperty("OutputParametersOnly", true);

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    TS_ASSERT(!Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Workspace"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Parameters"));

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_createDomain_creates_FunctionDomain1DSpectrum() {
    MatrixWorkspace_sptr ws2 = createTestWorkspace(true);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw;
    fitmw.setWorkspace(ws2);
    fitmw.setWorkspaceIndex(1);
    fitmw.createDomain(domain, values);

    FunctionDomain1DSpectrum *specDom =
        dynamic_cast<FunctionDomain1DSpectrum *>(domain.get());
    TS_ASSERT(specDom);
    TS_ASSERT_EQUALS(specDom->getWorkspaceIndex(), 1);
    TS_ASSERT_EQUALS(specDom->size(), ws2->blocksize());
  }

  void test_normalise_data() {
    MatrixWorkspace_sptr ws = createTestWorkspace(true);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    { // normalise the data
      FitMW fitmw;
      fitmw.setWorkspace(ws);
      fitmw.setWorkspaceIndex(1);
      fitmw.setNormalise(true);
      fitmw.createDomain(domain, values);

      auto &y = ws->y(1);

      for (size_t i = 0; i < values->size(); ++i) {
        TS_ASSERT_DELTA((*values).getFitData(i), y[i] / 0.1, 1e-8);
      }
    }

    { // don't normalise the data
      FitMW fitmw;
      fitmw.setWorkspace(ws);
      fitmw.setWorkspaceIndex(1);
      fitmw.setNormalise(false);
      fitmw.createDomain(domain, values);

      auto &y = ws->readY(1);

      for (size_t i = 0; i < values->size(); ++i) {
        TS_ASSERT_DELTA((*values).getFitData(i), y[i], 1e-8);
      }
    }
  }

  void test_create_SeqDomain() {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(2, 11, 10);

    for (size_t is = 0; is < ws2->getNumberHistograms(); ++is) {
      auto &x = ws2->mutableX(is);
      auto &y = ws2->mutableY(is);
      // Mantid::MantidVec& e = ws2->dataE(is);
      for (size_t i = 0; i < y.size(); ++i) {
        x[i] = 0.1 * double(i);
        if (i < 3) {
          y[i] = 1.0;
        } else if (i < 6) {
          y[i] = 2.0;
        } else if (i < 9) {
          y[i] = 3.0;
        } else {
          y[i] = 4.0;
        }
      }
      x.back() = x[x.size() - 2] + 0.1;
    }

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(FitMW::Sequential);
    fitmw.setWorkspace(ws2);
    fitmw.setWorkspaceIndex(0);
    fitmw.setMaxSize(3);
    fitmw.createDomain(domain, values);

    SeqDomain *seq = dynamic_cast<SeqDomain *>(domain.get());
    TS_ASSERT(seq);
    TS_ASSERT_EQUALS(seq->getNDomains(), 4);
    TS_ASSERT_EQUALS(seq->size(), 10);

    FunctionDomain_sptr d;
    FunctionValues_sptr v;
    seq->getDomainAndValues(0, d, v);
    TS_ASSERT_EQUALS(d->size(), 3);
    TS_ASSERT_EQUALS(v->size(), 3);
    auto d1d = static_cast<Mantid::API::FunctionDomain1D *>(d.get());
    auto v1d = static_cast<Mantid::API::FunctionValues *>(v.get());
    TS_ASSERT(d1d);
    TS_ASSERT_DELTA((*d1d)[0], 0.05, 1e-13);
    TS_ASSERT_DELTA((*d1d)[1], 0.15, 1e-13);
    TS_ASSERT_DELTA((*d1d)[2], 0.25, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(0), 1.0, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(1), 1.0, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(2), 1.0, 1e-13);
    v.reset();
    seq->getDomainAndValues(1, d, v);
    TS_ASSERT_EQUALS(d->size(), 3);
    TS_ASSERT_EQUALS(v->size(), 3);
    d1d = static_cast<Mantid::API::FunctionDomain1D *>(d.get());
    v1d = static_cast<Mantid::API::FunctionValues *>(v.get());
    TS_ASSERT(d1d);
    TS_ASSERT_DELTA((*d1d)[0], 0.35, 1e-13);
    TS_ASSERT_DELTA((*d1d)[1], 0.45, 1e-13);
    TS_ASSERT_DELTA((*d1d)[2], 0.55, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(0), 2.0, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(1), 2.0, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(2), 2.0, 1e-13);
    v.reset();
    seq->getDomainAndValues(2, d, v);
    TS_ASSERT_EQUALS(d->size(), 3);
    TS_ASSERT_EQUALS(v->size(), 3);
    d1d = static_cast<Mantid::API::FunctionDomain1D *>(d.get());
    v1d = static_cast<Mantid::API::FunctionValues *>(v.get());
    TS_ASSERT(d1d);
    TS_ASSERT_DELTA((*d1d)[0], 0.65, 1e-13);
    TS_ASSERT_DELTA((*d1d)[1], 0.75, 1e-13);
    TS_ASSERT_DELTA((*d1d)[2], 0.85, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(0), 3.0, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(1), 3.0, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(2), 3.0, 1e-13);
    v.reset();
    seq->getDomainAndValues(3, d, v);
    TS_ASSERT_EQUALS(d->size(), 1);
    TS_ASSERT_EQUALS(v->size(), 1);
    d1d = static_cast<Mantid::API::FunctionDomain1D *>(d.get());
    v1d = static_cast<Mantid::API::FunctionValues *>(v.get());
    TS_ASSERT(d1d);
    TS_ASSERT_DELTA((*d1d)[0], 0.95, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(0), 4.0, 1e-13);
  }

  void
  test_Composite_Function_With_SeparateMembers_Option_On_FitMW_Outputs_Composite_Values_Plus_Each_Member() {
    const bool histogram = true;
    auto ws2 = createTestWorkspace(histogram);
    const std::string inputWSName = "FitMWTest_CompositeTest";
    // AnalysisDataService::Instance().add(inputWSName, ws2);

    auto composite = boost::make_shared<API::CompositeFunction>();
    API::IFunction_sptr expDecay(new ExpDecay);
    expDecay->setParameter("Height", 1.5);
    expDecay->setError(0, 0.01);
    expDecay->setParameter("Lifetime", 2.0);
    expDecay->setError(1, 0.005);
    composite->addFunction(expDecay);
    expDecay = API::IFunction_sptr(new ExpDecay);
    expDecay->setParameter("Height", 2.0);
    expDecay->setError(0, 0.015);
    expDecay->setParameter("Lifetime", 3.0);
    expDecay->setError(1, 0.02);
    composite->addFunction(expDecay);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    // Requires a property manager to make a workspce
    auto propManager = boost::make_shared<Mantid::Kernel::PropertyManager>();
    const std::string wsPropName = "TestWorkspaceInput";
    propManager->declareProperty(
        Kernel::make_unique<WorkspaceProperty<Workspace>>(
            wsPropName, "", Mantid::Kernel::Direction::Input));
    propManager->setProperty<Workspace_sptr>(wsPropName, ws2);

    FitMW fitmw(propManager.get(), wsPropName);
    fitmw.declareDatasetProperties("", true);
    fitmw.initFunction(composite);
    fitmw.separateCompositeMembersInOutput(true);
    fitmw.createDomain(domain, values);

    // Create Output
    const std::string baseName("TestOutput_");
    fitmw.createOutputWorkspace(baseName, composite, domain, values,
                                "OutputWorkspace");

    MatrixWorkspace_sptr outputWS;
    // A new property should have appeared
    TS_ASSERT_THROWS_NOTHING(outputWS =
                                 propManager->getProperty("OutputWorkspace"));
    if (!outputWS) {
      TS_FAIL("No output workspace was found in the property manager");
      return;
    }

    static const size_t nExpectedHist(5);
    TS_ASSERT_EQUALS(nExpectedHist, outputWS->getNumberHistograms());
    // Check axis has expected labels
    API::Axis *axis = outputWS->getAxis(1);
    TS_ASSERT(axis);
    TS_ASSERT(axis->isText());
    TS_ASSERT_EQUALS(axis->length(), nExpectedHist);
    TS_ASSERT_EQUALS(axis->label(0), "Data");
    TS_ASSERT_EQUALS(axis->label(1), "Calc");
    TS_ASSERT_EQUALS(axis->label(2), "Diff");
    TS_ASSERT_EQUALS(axis->label(3), "ExpDecay");
    TS_ASSERT_EQUALS(axis->label(4), "ExpDecay");

    const double eValues[nExpectedHist] = {1.0, 0.01703318673, 0.0, 0.0092811,
                                           0.0142825267};
    const double yValues[nExpectedHist] = {
        8.1873075308, 3.294074078, 4.893233452, 1.391615229, 1.902458849};

    for (size_t i = 0; i < nExpectedHist; ++i) {
      TS_ASSERT_DELTA(outputWS->y(i)[1], yValues[i], 1e-8);
      TS_ASSERT_DELTA(outputWS->e(i)[1], eValues[i], 1e-8);
      TS_ASSERT_DELTA(outputWS->x(i)[1], ws2->readX(0)[1], 1e-8);
    }
  }

  void test_ignore_invalid_data() {
    auto ws = createTestWorkspace(false);
    const double one = 1.0;
    ws->dataY(0)[3] = std::numeric_limits<double>::infinity();
    ws->dataY(0)[5] = log(-one);
    ws->dataE(0)[7] = 0;
    ws->dataE(0)[9] = std::numeric_limits<double>::infinity();
    ws->dataE(0)[11] = log(-one);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    // Requires a property manager to make a workspce
    auto propManager = boost::make_shared<Mantid::Kernel::PropertyManager>();
    const std::string wsPropName = "TestWorkspaceInput";
    propManager->declareProperty(
        Kernel::make_unique<WorkspaceProperty<Workspace>>(
            wsPropName, "", Mantid::Kernel::Direction::Input));
    propManager->setProperty<Workspace_sptr>(wsPropName, ws);

    FitMW fitmw(propManager.get(), wsPropName);
    fitmw.declareDatasetProperties("", true);
    fitmw.ignoreInvalidData(true);
    fitmw.createDomain(domain, values);

    FunctionValues *val = dynamic_cast<FunctionValues *>(values.get());
    for (size_t i = 0; i < val->size(); ++i) {
      if (i == 3 || i == 5 || i == 7 || i == 9 || i == 11) {
        TS_ASSERT_EQUALS(val->getFitWeight(i), 0.0);
      } else {
        TS_ASSERT_DIFFERS(val->getFitWeight(i), 0.0);
      }
    }

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 1.);
    fun->setParameter("Lifetime", 1.0);

    Fit fit;
    fit.initialize();

    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("WorkspaceIndex", 0);

    fit.execute();
    TS_ASSERT(!fit.isExecuted());

    fit.setProperty("IgnoreInvalidData", true);
    fit.setProperty("Minimizer", "Levenberg-Marquardt");
    fit.execute();
    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);

    // check Levenberg-MarquardtMD minimizer
    fun->setParameter("Height", 1.);
    fun->setParameter("Lifetime", 1.0);
    Fit fit1;
    fit1.initialize();
    fit1.setProperty("Function", fun);
    fit1.setProperty("InputWorkspace", ws);
    fit1.setProperty("WorkspaceIndex", 0);
    fit1.setProperty("IgnoreInvalidData", true);
    fit1.setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit1.execute();
    TS_ASSERT(fit1.isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);
  }

  void test_setting_instrument_fitting_parameters() {
    boost::shared_ptr<Mantid::Geometry::Instrument> instrument;
    instrument.reset(new Mantid::Geometry::Instrument);
    Mantid::Geometry::ObjComponent *source =
        new Mantid::Geometry::ObjComponent("source");
    source->setPos(0.0, 0.0, -10.0);
    instrument->markAsSource(source);
    Mantid::Geometry::ObjComponent *sample =
        new Mantid::Geometry::ObjComponent("sample");
    instrument->markAsSamplePos(sample);
    auto *det = new Mantid::Geometry::Detector("det", 1, nullptr);
    instrument->add(det);
    instrument->markAsDetector(det);

    API::MatrixWorkspace_sptr ws = createTestWorkspace(false);
    ws->setInstrument(instrument);
    ws->getSpectrum(0).setDetectorID(det->getID());

    auto &pmap = ws->instrumentParameters();

    std::string value = "20.0 , ExpDecay , Lifetime , , , , , , , TOF ,";
    pmap.add("fitting", det, "Lifetime", value);
    const auto &detectorInfo = ws->detectorInfo();
    const auto detIndex = detectorInfo.indexOf(det->getID());
    const auto &detFromWS = detectorInfo.detector(detIndex);

    Geometry::Parameter_sptr param =
        pmap.getRecursive(&detFromWS, "Lifetime", "fitting");
    TS_ASSERT(param);

    API::IFunction_sptr expDecay(new ExpDecay);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    // Requires a property manager to make a workspce
    auto propManager = boost::make_shared<Mantid::Kernel::PropertyManager>();
    const std::string wsPropName = "TestWorkspaceInput";
    propManager->declareProperty(
        Kernel::make_unique<WorkspaceProperty<Workspace>>(
            wsPropName, "", Mantid::Kernel::Direction::Input));
    propManager->setProperty<Workspace_sptr>(wsPropName, ws);

    FitMW fitmw(propManager.get(), wsPropName);
    fitmw.declareDatasetProperties("", true);
    fitmw.createDomain(domain, values);
    fitmw.initFunction(expDecay);

    // test that the Lifetime parameter value was picked up from the instrument
    // parameter map
    TS_ASSERT_EQUALS(expDecay->getParameter("Lifetime"), 20.0);
  }

  void do_test_convolve_members_option(bool withBackground) {
    auto conv = boost::make_shared<Convolution>();
    auto resolution = IFunction_sptr(new Gaussian);
    resolution->initialize();
    resolution->setParameter("Height", 1.0);
    resolution->setParameter("PeakCentre", 0.0);
    resolution->setParameter("Sigma", 1.0);
    auto gaussian1 = IFunction_sptr(new Gaussian);
    gaussian1->initialize();
    gaussian1->setParameter("Height", 1.0);
    gaussian1->setParameter("PeakCentre", 0.0);
    gaussian1->setParameter("Sigma", 1.0);
    auto gaussian2 = IFunction_sptr(new Gaussian);
    gaussian2->initialize();
    gaussian2->setParameter("Height", 1.0);
    gaussian2->setParameter("PeakCentre", 1.0);
    gaussian2->setParameter("Sigma", 1.0);

    conv->addFunction(resolution);
    conv->addFunction(gaussian1);
    conv->addFunction(gaussian2);

    // workspace with 100 points on interval -10 <= x <= 10
    boost::shared_ptr<WorkspaceTester> data =
        boost::make_shared<WorkspaceTester>();
    data->initialize(1, 100, 100);

    auto &xData = data->mutableX(0);
    for (size_t i = 0; i < xData.size(); i++) {
      xData[i] = -10.0 + 0.2 * double(i);
    }

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    // Requires a property manager to make a workspce
    auto propManager = boost::make_shared<Mantid::Kernel::PropertyManager>();
    const std::string wsPropName = "TestWorkspaceInput";
    propManager->declareProperty(
        Kernel::make_unique<WorkspaceProperty<Workspace>>(
            wsPropName, "", Mantid::Kernel::Direction::Input));
    propManager->setProperty<Workspace_sptr>(wsPropName, data);

    IFunction_sptr fitfun;
    if (withBackground) {
      API::IFunction_sptr bckgd(new ExpDecay);
      bckgd->setParameter("Height", 1.);
      bckgd->setParameter("Lifetime", 1.);
      auto composite = boost::make_shared<API::CompositeFunction>();
      composite->addFunction(bckgd);
      composite->addFunction(conv);
      fitfun = composite;
    } else {
      fitfun = conv;
    }

    FitMW fitmw(propManager.get(), wsPropName);
    fitmw.declareDatasetProperties("", true);
    fitmw.initFunction(fitfun);
    fitmw.separateCompositeMembersInOutput(true, true);
    fitmw.createDomain(domain, values);

    // Create Output
    const std::string baseName("TestOutput_");
    fitmw.createOutputWorkspace(baseName, conv, domain, values,
                                "OutputWorkspace");

    MatrixWorkspace_sptr outputWS;
    // A new property should have appeared
    TS_ASSERT_THROWS_NOTHING(outputWS =
                                 propManager->getProperty("OutputWorkspace"));
    if (!outputWS) {
      TS_FAIL("No output workspace was found in the property manager");
      return;
    }

    static const size_t nExpectedHist(5);
    TS_ASSERT_EQUALS(nExpectedHist, outputWS->getNumberHistograms());
    // Check axis has expected labels
    API::Axis *axis = outputWS->getAxis(1);
    TS_ASSERT(axis);
    TS_ASSERT(axis->isText());
    TS_ASSERT_EQUALS(axis->length(), nExpectedHist);
    TS_ASSERT_EQUALS(axis->label(0), "Data");
    TS_ASSERT_EQUALS(axis->label(1), "Calc");
    TS_ASSERT_EQUALS(axis->label(2), "Diff");
    TS_ASSERT_EQUALS(axis->label(3), "Convolution");
    TS_ASSERT_EQUALS(axis->label(4), "Convolution");

    FunctionDomain1DView x(data->mutableX(0).rawData().data(),
                           data->mutableX(0).size());
    FunctionValues gaus1Values(x);
    FunctionValues gaus2Values(x);

    Convolution conv1;
    conv1.addFunction(resolution);
    conv1.addFunction(gaussian1);
    conv1.function(x, gaus1Values);

    const size_t numBins = data->blocksize();
    for (size_t i = 0; i < numBins; i++) {
      TS_ASSERT_EQUALS(outputWS->y(3)[i], gaus1Values[i]);
      TS_ASSERT_DIFFERS(outputWS->y(3)[i], 0.0);
    }

    Convolution conv2;
    conv2.addFunction(resolution);
    conv2.addFunction(gaussian2);
    conv2.function(x, gaus2Values);

    for (size_t i = 0; i < numBins; i++) {
      TS_ASSERT_EQUALS(outputWS->y(4)[i], gaus2Values[i]);
      TS_ASSERT_DIFFERS(outputWS->y(4)[i], 0.0);
      TS_ASSERT_DIFFERS(outputWS->y(4)[i], outputWS->y(3)[i]);
    }
  }

  void test_convolve_members_option_without_background() {
    do_test_convolve_members_option(false);
  }

  void test_convolve_members_option_with_background() {
    do_test_convolve_members_option(true);
  }

  void test_exclude() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) {
          if (x >= 1.0 && x <= 2.0)
            return 2.0;
          return 1.0;
        },
        1, 0.0, 3., 0.5);

    std::vector<double> exclude{1.0, 2.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit.execute();

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1);
  }

  void test_exclude_1() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) {
          if (x >= 1.0 && x <= 2.0)
            return 2.0;
          return 1.0;
        },
        1, 0.0, 3., 0.5);

    std::vector<double> exclude{-2.0, -1.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit.execute();

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("A0"), 1.4285, 1e-4);
  }

  void test_exclude_2() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) {
          if (x >= 1.0 && x <= 2.0)
            return 2.0;
          return 1.0;
        },
        1, 0.0, 3., 0.5);

    std::vector<double> exclude{4.0, 5.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit.execute();

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("A0"), 1.4285, 1e-4);
  }

  void test_exclude_3() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) {
          if (x >= 1.0 && x <= 2.0)
            return 2.0;
          return 1.0;
        },
        1, 0.0, 3., 0.5);

    std::vector<double> exclude{-2.0, -1.0, 4.0, 5.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit.execute();

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("A0"), 1.4285, 1e-4);
  }

  void test_exclude_4() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) {
          if (x >= 0.0 && x <= 1.0)
            return 2.0;
          if (x >= 4.0 && x <= 6.0)
            return 3.0;
          if (x >= 9.0 && x <= 10.0)
            return 4.0;
          return 1.0;
        },
        1, 0.0, 10., 1.0);

    std::vector<double> exclude{-1.0, 1.0, 4.0, 6.5, 9.0, 11.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);
    fit.setProperty("Output", "out");

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0);

    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1);

    auto out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "out_Workspace");
    TS_ASSERT(out);
    if (!out)
      return;

    auto &diff = out->y(2);
    for (size_t i = 0; i < diff.size(); ++i) {
      TS_ASSERT_EQUALS(diff[i], 0.0);
    }
  }

  void test_exclude_odd_number() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 1.0; }, 1, 0.0, 3., 0.5);

    std::vector<double> exclude{-2.0, -1.0, 4.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(fit.setProperty("Exclude", exclude),
                     std::invalid_argument);
  }

  void test_exclude_unordered() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 1.0; }, 1, 0.0, 3., 0.5);

    std::vector<double> exclude{-2.0, -1.0, 4.0, 2.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(fit.setProperty("Exclude", exclude),
                     std::invalid_argument);
  }

  void test_exclude_overlapped() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{-1.0, 1.0, 0.0, 1.0, 9.0, 11.0, 7.5, 9.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0);
  }

  void test_exclude_overlapped_1() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{6.0, 8.1, 2.1, 4.3, 4.2, 6.7};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 1);
  }

  void test_exclude_overlapped_2() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{0.0, 2.0, 2.0, 4.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 1);
  }

  void test_exclude_jump_into_excluded() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{0.0, 4.1, 4.3, 11.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0);
  }

  void test_exclude_jump_over_few_ranges() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{0.0, 4.1, 4.2, 4.3, 4.4, 4.5, 6.0, 11.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0);
  }

  void test_exclude_jump_over_few_ranges_into_excluded() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{0.0, 4.1, 4.2, 4.3, 4.4, 4.5, 4.9, 11.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0);
  }

  void test_exclude_almost_empty_range() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{0.0, 4.1, 5.0, 5.0, 8.0, 10.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0);
  }

  void test_exclude_almost_empty_range_1() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 10., 1.0);

    std::vector<double> exclude{0.0, 4.1, 5.0, 5.0};
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Exclude", exclude);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    FitMW fitmw(&fit, "InputWorkspace");
    fitmw.declareDatasetProperties("", false);
    fitmw.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 1);
  }
};

class FitMWTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitMWTestPerformance *createSuite() {
    return new FitMWTestPerformance();
  }
  static void destroySuite(FitMWTestPerformance *suite) { delete suite; }

  FitMWTestPerformance() {
    bool histogram = false;
    binsWS = createTestWorkspace(histogram, NVectors, YLength);

    histogram = true;
    histWS = createTestWorkspace(histogram, NVectors, YLength);
  }

  void test_exec_point_data_performance() {
    doTestExecPointData(binsWS, performance);
  }
  void test_exec_histogram_data_performance() {
    doTestExecHistogramData(histWS, performance);
  }

private:
  API::MatrixWorkspace_sptr histWS;
  API::MatrixWorkspace_sptr binsWS;
  const bool performance = true;
  const size_t NVectors = 2000;
  const size_t YLength = 10000;
};
#endif /*CURVEFITTING_FITMWTEST_H_*/
