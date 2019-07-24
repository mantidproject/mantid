// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_TABLEWORKSPACEDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_TABLEWORKSPACEDOMAINCREATORTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidCurveFitting/Functions/FlatBackground.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Polynomial.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/TableWorkspaceDomainCreator.h"

using Mantid::CurveFitting::TableWorkspaceDomainCreator;
using namespace Mantid;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

class TableWorkspaceDomainCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TableWorkspaceDomainCreatorTest *createSuite() {
    return new TableWorkspaceDomainCreatorTest();
  }
  static void destroySuite(TableWorkspaceDomainCreatorTest *suite) {
    delete suite;
  }

  void test_exec_with_table_workspace() {
    auto ws = createTestTableWorkspace();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws, false);
    fit->execute();
    TS_ASSERT(fit->isExecuted());

    API::AnalysisDataService::Instance().clear();
  }

  void test_function_parameters() {
    auto ws = createTestTableWorkspace();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws, false);
    fit->execute();

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-6);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-6);

    API::AnalysisDataService::Instance().clear();
  }

  void test_Output_Workspace() {
    auto ws = createTestTableWorkspace();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->execute();

    TS_ASSERT_EQUALS(fit->getPropertyValue("OutputStatus"), "success");

    API::MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
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

    API::AnalysisDataService::Instance().clear();
  }

  void test_Output_NormalisedCovarianceMatrix_table() {
    auto ws = createTestTableWorkspace();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->execute();

    API::ITableWorkspace_sptr covar =
        boost::dynamic_pointer_cast<API::ITableWorkspace>(
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

    API::AnalysisDataService::Instance().clear();
  }

  void test_Output_Parameters_table() {
    auto ws = createTestTableWorkspace();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->execute();
    API::ITableWorkspace_sptr params =
        boost::dynamic_pointer_cast<API::ITableWorkspace>(
            API::AnalysisDataService::Instance().retrieve("Output_Parameters"));

    double chi2 = fit->getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-8);

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

    API::AnalysisDataService::Instance().clear();
  }

  // test that errors of the calculated output are reasonable
  void test_output_errors_are_reasonable() {
    auto ws = createTestTableWorkspace();
    auto fun = createPolynomialFunction(5);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->execute();

    TS_ASSERT(fit->isExecuted());

    Mantid::API::MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("Output_Workspace");
    TS_ASSERT(out_ws);
    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 3);
    auto &e = out_ws->e(1);
    for (size_t i = 0; i < e.size(); ++i) {
      TS_ASSERT(e[i] < 1.0);
    }

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_output_no_errors_provided() {
    auto wsWithNoErrors = createTestTableWorkspace(false);
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, wsWithNoErrors);
    fit->execute();

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-6);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-6);

    API::AnalysisDataService::Instance().clear();
  }

  void test_all_outputs() {
    auto ws = createTestTableWorkspace();
    auto fun = createPolynomialFunction(1);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Output", "out");
    fit->execute();

    TS_ASSERT(fit->isExecuted());
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Workspace"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Parameters"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_NormalisedCovarianceMatrix"));

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_output_parameters_only() {
    auto ws = createTestTableWorkspace();
    auto fun = createPolynomialFunction(1);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Output", "out");
    fit->setProperty("OutputParametersOnly", true);
    fit->execute();

    TS_ASSERT(fit->isExecuted());
    TS_ASSERT(!Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Workspace"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "out_Parameters"));

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_takes_correct_columns_when_given_column_names() {
    API::ITableWorkspace_sptr table =
        API::WorkspaceFactory::Instance().createTable();
    table->addColumn("double", "Y data");
    table->addColumn("double", "Errors");
    table->addColumn("double", "other data");
    table->addColumn("double", "X data");
    table->addColumn("double", "more extra data");

    for (auto i = 0; i != 20; ++i) {
      const double xValue = i * 0.1;
      const double yValue = 10.0 * exp(-xValue / 0.5);
      const double eValue = 0.1;
      API::TableRow newRow = table->appendRow();
      newRow << yValue << eValue << 2.0 << xValue << 5.1;
    }

    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, table);
    fit->setProperty("XColumnName", "X data");
    fit->setProperty("YColumnName", "Y data");
    fit->setProperty("ErrorColumnName", "Errors");
    fit->execute();

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-6);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-6);

    API::AnalysisDataService::Instance().clear();
  }

  void test_createDomain_creates_FunctionDomain1DVector() {
    auto ws = createTestTableWorkspace();

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableDomainCreator;
    tableDomainCreator.setWorkspace(ws);
    tableDomainCreator.createDomain(domain, values);

    API::FunctionDomain1DVector *specDom =
        dynamic_cast<API::FunctionDomain1DVector *>(domain.get());
    TS_ASSERT(specDom);
    TS_ASSERT_EQUALS(specDom->size(), ws->rowCount());
  }

  void test_create_SeqDomain_creates_domain() {
    auto ws = createTableWorkspaceForSeqFit();

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableDomainCreator(
        TableWorkspaceDomainCreator::Sequential);
    tableDomainCreator.setWorkspace(ws);
    tableDomainCreator.setMaxSize(3);
    tableDomainCreator.createDomain(domain, values);

    CurveFitting::SeqDomain *seq =
        dynamic_cast<CurveFitting::SeqDomain *>(domain.get());
    TS_ASSERT(seq);
    TS_ASSERT_EQUALS(seq->getNDomains(), 4);
    TS_ASSERT_EQUALS(seq->size(), 10);
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_create_SeqDomain_outputs() {
    auto ws = createTableWorkspaceForSeqFit();

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableDomainCreator(
        TableWorkspaceDomainCreator::Sequential);
    tableDomainCreator.setWorkspace(ws);
    tableDomainCreator.setMaxSize(3);
    tableDomainCreator.createDomain(domain, values);

    CurveFitting::SeqDomain *seq =
        dynamic_cast<CurveFitting::SeqDomain *>(domain.get());
    TS_ASSERT(seq);

    API::FunctionDomain_sptr dom;
    API::FunctionValues_sptr val;

    for (auto i = 0; i < 2; ++i) {
      seq->getDomainAndValues(i, dom, val);
      TS_ASSERT_EQUALS(dom->size(), 3);
      TS_ASSERT_EQUALS(val->size(), 3);
      auto d1d = static_cast<Mantid::API::FunctionDomain1D *>(dom.get());
      auto v1d = static_cast<Mantid::API::FunctionValues *>(val.get());
      TS_ASSERT(d1d);
      TS_ASSERT_DELTA((*d1d)[0], i * 0.3, 1e-13);
      TS_ASSERT_DELTA((*d1d)[1], 0.1 + i * 0.3, 1e-13);
      TS_ASSERT_DELTA((*d1d)[2], 0.2 + i * 0.3, 1e-13);
      TS_ASSERT_DELTA(v1d->getFitData(0), i + 1, 1e-13);
      TS_ASSERT_DELTA(v1d->getFitData(1), i + 1, 1e-13);
      TS_ASSERT_DELTA(v1d->getFitData(2), i + 1, 1e-13);
      val.reset();
    }
    seq->getDomainAndValues(3, dom, val);
    TS_ASSERT_EQUALS(dom->size(), 1);
    TS_ASSERT_EQUALS(val->size(), 1);
    auto d1d = static_cast<Mantid::API::FunctionDomain1D *>(dom.get());
    auto v1d = static_cast<Mantid::API::FunctionValues *>(val.get());
    TS_ASSERT(d1d);
    TS_ASSERT_DELTA((*d1d)[0], 0.9, 1e-13);
    TS_ASSERT_DELTA(v1d->getFitData(0), 4.0, 1e-13);

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_ignore_invalid_data_weighting() {
    auto ws = createTableWorkspaceWithInvalidData();

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    // Requires a property manager to make a workspce
    auto propManager = boost::make_shared<Mantid::Kernel::PropertyManager>();
    const std::string wsPropName = "TestWorkspaceInput";
    propManager->declareProperty(
        std::make_unique<API::WorkspaceProperty<API::Workspace>>(
            wsPropName, "", Mantid::Kernel::Direction::Input));
    propManager->setProperty<API::Workspace_sptr>(wsPropName, ws);

    TableWorkspaceDomainCreator tableWSDomainCreator(propManager.get(),
                                                     wsPropName);
    tableWSDomainCreator.declareDatasetProperties("", true);
    tableWSDomainCreator.ignoreInvalidData(true);
    tableWSDomainCreator.createDomain(domain, values);

    API::FunctionValues *val =
        dynamic_cast<API::FunctionValues *>(values.get());
    for (size_t i = 0; i < val->size(); ++i) {
      if (i == 3 || i == 5 || i == 7 || i == 9 || i == 11) {
        TS_ASSERT_EQUALS(val->getFitWeight(i), 0.0);
      } else {
        TS_ASSERT_DIFFERS(val->getFitWeight(i), 0.0);
      }
    }
    API::AnalysisDataService::Instance().clear();
  }

  void test_ignore_invalid_data_LevenbergMarquardt() {
    auto ws = createTableWorkspaceWithInvalidData();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("IgnoreInvalidData", true);
    fit->setProperty("Minimizer", "Levenberg-Marquardt");
    fit->execute();
    TS_ASSERT(fit->isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_ignore_invalid_data_LevenbergMarquardtMD() {
    auto ws = createTableWorkspaceWithInvalidData();
    auto fun = createExpDecayFunction(1.0, 1.0);
    auto fit1 = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit1->setProperty("IgnoreInvalidData", true);
    fit1->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit1->execute();
    TS_ASSERT(fit1->isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);

    Mantid::API::AnalysisDataService::Instance().clear();

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_with_values_in_data() {
    auto ws = createTableWorkspaceForExclude();

    std::vector<double> exclude{1.0, 2.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Exclude", exclude);

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableWSDomainCreator(fit.get(),
                                                     "InputWorkspace");
    tableWSDomainCreator.declareDatasetProperties("", false);
    tableWSDomainCreator.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit->execute();

    fun = fit->getProperty("Function");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_with_values_below_x_data_range() {
    auto ws = createTableWorkspaceForExclude();
    std::vector<double> exclude{-2.0, -1.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Exclude", exclude);

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableWSDomainCreator(fit.get(),
                                                     "InputWorkspace");
    tableWSDomainCreator.declareDatasetProperties("", false);
    tableWSDomainCreator.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit->execute();

    fun = fit->getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("A0"), 1.4285, 1e-4);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_with_values_above_x_data_range() {
    auto ws = createTableWorkspaceForExclude();
    std::vector<double> exclude{4.0, 5.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Exclude", exclude);

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableWSDomainCreator(fit.get(),
                                                     "InputWorkspace");
    tableWSDomainCreator.declareDatasetProperties("", false);
    tableWSDomainCreator.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit->execute();

    fun = fit->getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("A0"), 1.4285, 1e-4);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_with_values_above_and_below_x_data_range() {
    auto ws = createTableWorkspaceForExclude();
    std::vector<double> exclude{-2.0, -1.0, 4.0, 5.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Exclude", exclude);

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableWSDomainCreator(fit.get(),
                                                     "InputWorkspace");
    tableWSDomainCreator.declareDatasetProperties("", false);
    tableWSDomainCreator.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    fit->execute();

    fun = fit->getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("A0"), 1.4285, 1e-4);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_fails_with_odd_number_of_entries() {
    auto ws = createTableWorkspaceForExclude();
    std::vector<double> exclude{-2.0, -1.0, 4.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);

    TS_ASSERT_THROWS(fit->setProperty("Exclude", exclude),
                     const std::invalid_argument &);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_fails_for_unordered_entries() {
    auto ws = createTableWorkspaceForExclude();
    std::vector<double> exclude{-2.0, -1.0, 4.0, 2.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    TS_ASSERT_THROWS(fit->setProperty("Exclude", exclude),
                     const std::invalid_argument &);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_for_overlapped_entries() {
    auto ws = createTableWorkspaceForExclude();

    std::vector<double> exclude{-1.0, 0.5, 0.0, 0.5, 2.5, 5.0, 2.0, 4.0};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Exclude", exclude);

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableWSDomainCreator(fit.get(),
                                                     "InputWorkspace");
    tableWSDomainCreator.declareDatasetProperties("", false);
    tableWSDomainCreator.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0);

    API::AnalysisDataService::Instance().clear();
  }

  void test_exclude_overlapped_unsorted_order() {
    auto ws = createTableWorkspaceForExclude();

    std::vector<double> exclude{2.2, 2.9, 0.6, 1.5, 1.4, 2.4};
    API::IFunction_sptr fun(new FlatBackground);
    fun->initialize();
    auto fit = setupBasicFitPropertiesAlgorithm(fun, ws);
    fit->setProperty("Exclude", exclude);

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;

    TableWorkspaceDomainCreator tableWSDomainCreator(fit.get(),
                                                     "InputWorkspace");
    tableWSDomainCreator.declareDatasetProperties("", false);
    tableWSDomainCreator.createDomain(domain, values);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 1);

    API::AnalysisDataService::Instance().clear();
  }

private:
  API::ITableWorkspace_sptr
  createEmptyTableWith3ColumnsWorkspace(bool errors = true) {
    API::ITableWorkspace_sptr table =
        API::WorkspaceFactory::Instance().createTable();
    table->addColumn("double", "X data");
    table->addColumn("double", "Y data");
    if (errors)
      table->addColumn("double", "Errors");
    return table;
  }

  API::ITableWorkspace_sptr createTestTableWorkspace(bool errors = true) {
    auto table = createEmptyTableWith3ColumnsWorkspace(errors);

    for (auto i = 0; i != 20; ++i) {
      const double xValue = i * 0.1;
      const double yValue = 10.0 * exp(-xValue / 0.5);
      API::TableRow newRow = table->appendRow();
      if (errors) {
        const double eValue = 0.1;
        newRow << xValue << yValue << eValue;
      } else {
        newRow << xValue << yValue;
      }
    }

    return table;
  }

  API::ITableWorkspace_sptr createTableWorkspaceForSeqFit() {
    auto table = createEmptyTableWith3ColumnsWorkspace();

    for (auto i = 0; i != 10; ++i) {
      const double xValue = i * 0.1;
      double yValue;
      if (i < 3) {
        yValue = 1.0;
      } else if (i < 6) {
        yValue = 2.0;
      } else if (i < 9) {
        yValue = 3.0;
      } else {
        yValue = 4.0;
      }
      const double eValue = 0.1;
      API::TableRow newRow = table->appendRow();
      newRow << xValue << yValue << eValue;
    }
    return table;
  }

  API::ITableWorkspace_sptr createTableWorkspaceWithInvalidData() {
    auto table = createEmptyTableWith3ColumnsWorkspace();

    for (auto i = 0; i != 20; ++i) {
      const double xValue = i * 0.1;
      double yValue;
      double eValue;
      const double one = 1.0;

      if (i == 3)
        yValue = std::numeric_limits<double>::infinity();
      else if (i == 5)
        yValue = log(-one);
      else
        yValue = 10.0 * exp(-xValue / 0.5);

      if (i == 7)
        eValue = 0;
      else if (i == 9)
        eValue = std::numeric_limits<double>::infinity();
      else if (i == 11)
        eValue = log(-one);
      else
        eValue = 0.1;

      API::TableRow newRow = table->appendRow();
      newRow << xValue << yValue << eValue;
    }
    return table;
  }

  API::ITableWorkspace_sptr createTableWorkspaceForExclude() {
    auto table = createEmptyTableWith3ColumnsWorkspace();
    for (auto i = 0; i < 7; ++i) {
      const double xValue = i * 0.5;
      double yValue;
      if (xValue >= 1.0 && xValue <= 2.0)
        yValue = 2.0;
      else
        yValue = 1.0;
      API::TableRow newRow = table->appendRow();
      newRow << xValue << yValue;
    }
    return table;
  }

  API::IFunction_sptr createExpDecayFunction(double height, double lifetime) {
    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", height);
    fun->setParameter("Lifetime", lifetime);
    return fun;
  }

  API::IFunction_sptr createPolynomialFunction(int degree) {
    API::IFunction_sptr fun(new Polynomial);
    fun->setAttributeValue("n", degree);
    return fun;
  }

  API::IFunction_sptr createGaussianFunction(double height, double peakCentre,
                                             double sigma) {
    API::IFunction_sptr fun(new Gaussian);
    fun->initialize();
    fun->setParameter("Height", height);
    fun->setParameter("PeakCentre", peakCentre);
    fun->setParameter("Sigma", sigma);
    return fun;
  }

  boost::shared_ptr<Fit>
  setupBasicFitPropertiesAlgorithm(API::IFunction_sptr fun,
                                   API::Workspace_sptr ws,
                                   bool createOutput = true) {
    auto fit = boost::make_shared<Fit>();
    fit->initialize();
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CreateOutput", createOutput);

    return fit;
  }
};

#endif /* MANTID_CURVEFITTING_TABLEWORKSPACEDOMAINCREATORTEST_H_ */
