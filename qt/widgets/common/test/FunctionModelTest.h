// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <cxxtest/TestSuite.h>

#include <memory>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

QList<FunctionModelDataset> createDatasets(QStringList const &datasetNames, std::string const &spectraString) {
  QList<FunctionModelDataset> datasets;
  for (const auto &datasetName : datasetNames)
    datasets.append(FunctionModelDataset(datasetName, FunctionModelSpectra(spectraString)));
  return datasets;
}

} // namespace

class FunctionModelTest : public CxxTest::TestSuite {

public:
  static FunctionModelTest *createSuite() { return new FunctionModelTest; }
  static void destroySuite(FunctionModelTest *suite) { delete suite; }

  FunctionModelTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void setUp() override { m_model = std::make_unique<FunctionModel>(); }

  void tearDown() override { m_model.reset(); }

  void test_empty() { TS_ASSERT(!m_model->getFitFunction()); }

  void test_simple() {
    m_model->setFunctionString("name=LinearBackground,A0=1,A1=2");
    auto fun = m_model->getFitFunction();
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
    TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
  }

  void test_simple_multidomain() {
    m_model->setFunctionString("name=LinearBackground,A0=1,A1=2");
    m_model->setNumberDomains(2);
    TS_ASSERT_EQUALS(m_model->getNumberDomains(), 2);
    TS_ASSERT_EQUALS(m_model->currentDomainIndex(), 0);
    m_model->setCurrentDomainIndex(1);
    TS_ASSERT_EQUALS(m_model->currentDomainIndex(), 1);
    TS_ASSERT_THROWS_EQUALS(m_model->setCurrentDomainIndex(2), std::runtime_error & e, std::string(e.what()),
                            "Domain index is out of range: 2 out of 2");
    {
      auto fun = m_model->getCurrentFunction();
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    {
      auto fun = m_model->getSingleFunction(0);
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    {
      auto fun = m_model->getSingleFunction(1);
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    TS_ASSERT_THROWS_EQUALS(m_model->getSingleFunction(2), std::runtime_error & e, std::string(e.what()),
                            "Domain index is out of range: 2 out of 2");
    {
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->name(), "MultiDomainFunction");
      TS_ASSERT_EQUALS(fun->getParameter("f0.A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("f0.A1"), 2.0);
      TS_ASSERT_EQUALS(fun->getParameter("f1.A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("f1.A1"), 2.0);
    }
  }

  void test_function_resolution_from_workspace() {
    FunctionModel model;
    auto algo = AlgorithmManager::Instance().create("Load");
    algo->setPropertyValue("Filename", "iris26173_graphite002_res");
    algo->setPropertyValue("OutputWorkspace", "iris26173_graphite002_res");
    algo->execute();
    auto initialFunString = "composite=Convolution,NumDeriv=true,FixResolution=true;name="
                            "Resolution,"
                            "Workspace=iris26173_graphite002_res,WorkspaceIndex=0,X=(),Y=();name="
                            "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<"
                            "FWHM)";
    auto correctedFunString = "composite=Convolution,NumDeriv=true,FixResolution=true;name="
                              "Resolution,"
                              "Workspace=iris26173_graphite002_res,WorkspaceIndex=0,X=(),Y=();name="
                              "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175,constraints=(0<"
                              "Amplitude,0<"
                              "FWHM)";
    model.setFunctionString(initialFunString);
    auto fun = model.getFitFunction();
    auto funString = fun->asString();
    TS_ASSERT(funString == correctedFunString);
  }

  void test_globals() {
    m_model->setFunctionString("name=LinearBackground,A0=1,A1=2");
    m_model->setNumberDomains(3);
    QStringList globals("A1");
    m_model->setGlobalParameters(globals);
    auto fun = m_model->getFitFunction();
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT_EQUALS(fun->getTie(3)->asString(), "f1.A1=f0.A1");
    TS_ASSERT_EQUALS(fun->getTie(5)->asString(), "f2.A1=f0.A1");
    auto locals = m_model->getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A0");
    globals.clear();
    globals << "A0";
    m_model->setGlobalParameters(globals);
    fun = m_model->getFitFunction();
    TS_ASSERT(!fun->getTie(0));
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT(!fun->getTie(3));
    TS_ASSERT(!fun->getTie(5));
    TS_ASSERT_EQUALS(fun->getTie(2)->asString(), "f1.A0=f0.A0");
    TS_ASSERT_EQUALS(fun->getTie(4)->asString(), "f2.A0=f0.A0");
    locals = m_model->getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A1");
  }

  void test_that_setParameter_will_set_a_local_parameter_as_expected() {
    m_model->setFunctionString("composite=MultiDomainFunction,NumDeriv=true;"
                               "name=LinearBackground,A0=1,A1=2,$domains=i;"
                               "name=LinearBackground,A0=1,A1=2,$domains=i");

    m_model->setNumberDomains(2);
    m_model->setCurrentDomainIndex(0);
    m_model->setParameter("A0", 5.0);

    TS_ASSERT_EQUALS(m_model->getFitFunction()->asString(), "composite=MultiDomainFunction,NumDeriv=true;"
                                                            "name=LinearBackground,A0=5,A1=2,$domains=i;"
                                                            "name=LinearBackground,A0=1,A1=2,$domains=i;"
                                                            "name=LinearBackground,A0=1,A1=2,$domains=All");
  }

  void test_that_setParameter_will_set_a_global_parameter_as_expected() {
    m_model->setFunctionString("composite=MultiDomainFunction,NumDeriv=true;"
                               "name=LinearBackground,A0=1,A1=2,$domains=i;"
                               "name=LinearBackground,A0=1,A1=2,$domains=i");

    m_model->setNumberDomains(2);
    m_model->setCurrentDomainIndex(0);
    m_model->setGlobalParameters(QStringList("A0"));
    m_model->setParameter("A0", 5.0);

    TS_ASSERT_EQUALS(m_model->getFitFunction()->asString(), "composite=MultiDomainFunction,NumDeriv=true;"
                                                            "name=LinearBackground,A0=5,A1=2,$domains=i;"
                                                            "name=LinearBackground,A0=5,A1=2,$domains=i;"
                                                            "name=LinearBackground,A0=5,A1=2,$domains=All;"
                                                            "ties=(f2.A0=f0.A0,f1.A0=f0.A0)");
  }

  void test_set_number_domains_after_clear() {
    m_model->clear();
    m_model->setNumberDomains(1);
    TS_ASSERT_EQUALS(m_model->getNumberDomains(), 1);
  }

  void test_add_function_top_level() {
    {
      m_model->addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=3,A1=4");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 2);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
    }
    {
      m_model->addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      m_model->addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6;"
          "name=LinearBackground,A0=7,A1=8");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
  }

  void test_add_function_nested() {
    m_model->addFunction("", "name=LinearBackground,A0=1,A1=2;(composite=CompositeFunction)");
    {
      m_model->addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      m_model->addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;"
          "(name=LinearBackground,A0=5,A1=6;name=LinearBackground,A0=7,A1=8)");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
    {
      m_model->addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;"
          "(name=LinearBackground,A0=5,A1=6;name=LinearBackground,A0=7,A1=8;"
          "name=LinearBackground,A0=9,A1=10)");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 8);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
      TS_ASSERT_EQUALS(fun->getParameter(6), 9.0);
      TS_ASSERT_EQUALS(fun->getParameter(7), 10.0);
    }
  }

  void test_remove_function() {
    m_model->addFunction("", "name=LinearBackground,A0=1,A1=2;name="
                             "LinearBackground,A0=1,A1=2;name=LinearBackground,A0="
                             "1,A1=2");
    {
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6;"
          "name=LinearBackground,A0=7,A1=8");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
    {
      m_model->removeFunction("f1.");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      m_model->removeFunction("f1.");
      auto testFun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=3,A1=4");
      m_model->updateMultiDatasetParameters(*testFun);
      auto fun = m_model->getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 2);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
    }
  }
  void test_getAttributeNames_returns_correctly() {
    m_model->addFunction("", "name = TeixeiraWaterSQE, Q = 3.14,"
                             "WorkspaceIndex = 4, Height = 1,"
                             "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                             "constraints=(Height>0, DiffCoeff>0, "
                             "Tau>0);name=FlatBackground;name=LinearBackground");
    QStringList expectedAttributes = {QString("NumDeriv"), QString("f0.Q"), QString("f0.WorkspaceIndex")};

    auto attributes = m_model->getAttributeNames();

    TS_ASSERT_EQUALS(expectedAttributes.size(), attributes.size());

    for (int i = 0; i < attributes.size(); ++i) {
      TS_ASSERT_EQUALS(attributes[i], expectedAttributes[i]);
    }
  }
  void test_setAttribute_correctly_updates_stored_function() {
    m_model->addFunction("", "name = TeixeiraWaterSQE, Q = 3.14,"
                             "WorkspaceIndex = 4, Height = 1,"
                             "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                             "constraints=(Height>0, DiffCoeff>0, "
                             "Tau>0);name=FlatBackground;name=LinearBackground");
    m_model->setAttribute("f0.Q", IFunction::Attribute(41.3));

    TS_ASSERT_EQUALS(m_model->getCurrentFunction()->getAttribute("f0.Q").asDouble(), 41.3);
  }
  void test_getAttribute_correctly_retrives_attributes() {
    m_model->addFunction("", "name=TeixeiraWaterSQE, Q = 3.14,"
                             "WorkspaceIndex = 4, Height = 1,"
                             "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                             "constraints=(Height>0, DiffCoeff>0, "
                             "Tau>0);name=FlatBackground;name=LinearBackground");
    TS_ASSERT_EQUALS(m_model->getAttribute("f0.Q").asDouble(), 3.14);
    TS_ASSERT_EQUALS(m_model->getAttribute("NumDeriv").asBool(), false);
  }
  void test_getAttribute_throws_for_non_exisitng_attribute() {
    m_model->addFunction("", "name=TeixeiraWaterSQE, Q = 3.14,"
                             "WorkspaceIndex = 4, Height = 1,"
                             "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                             "constraints=(Height>0, DiffCoeff>0, "
                             "Tau>0);name=FlatBackground;name=LinearBackground");
    TS_ASSERT_THROWS(m_model->getAttribute("f0.B").asDouble(), std::invalid_argument &);
  }
  void test_updateMultiDatasetAttributes_correctly_updates_stored_attributes() {
    m_model->setNumberDomains(3);
    m_model->addFunction("", "name=TeixeiraWaterSQE, Q = 3.14,"
                             "WorkspaceIndex = 4, Height = 1,"
                             "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                             "constraints=(Height>0, DiffCoeff>0, "
                             "Tau>0);name=FlatBackground;name=LinearBackground");
    auto function = FunctionFactory::Instance().createInitializedMultiDomainFunction(
        "name=TeixeiraWaterSQE, Q=41.3, "
        "Height=1, DiffCoeff=2.3, Tau=1.25, Centre=0, "
        "constraints=(Height>0, DiffCoeff>0, "
        "Tau>0);name=FlatBackground;name=LinearBackground",
        3);
    function->setAttribute("f0.f0.Q", IFunction::Attribute(11.3));
    function->setAttribute("f1.f0.Q", IFunction::Attribute(21.6));
    function->setAttribute("f2.f0.Q", IFunction::Attribute(32.9));

    m_model->updateMultiDatasetAttributes(*function);

    TS_ASSERT_EQUALS(m_model->getFitFunction()->getAttribute("f0.f0.Q").asDouble(), 11.3);
    TS_ASSERT_EQUALS(m_model->getFitFunction()->getAttribute("f1.f0.Q").asDouble(), 21.6);
    TS_ASSERT_EQUALS(m_model->getFitFunction()->getAttribute("f2.f0.Q").asDouble(), 32.9);
  }

  void test_that_getDatasetNames_returns_the_expected_workspace_names_for_single_spectra_workspaces() {
    auto const datasetNames = QStringList({"WSName1", "WSName2", "WSName3"});

    m_model->setNumberDomains(3);
    m_model->setDatasets(datasetNames);

    TS_ASSERT_EQUALS(m_model->getDatasetNames(), datasetNames);
  }

  void test_that_getDatasetDomainNames_returns_the_expected_domain_names_for_single_spectra_workspaces() {
    auto const datasetNames = QStringList({"WSName1", "WSName2", "WSName3"});

    m_model->setNumberDomains(3);
    m_model->setDatasets(datasetNames);

    TS_ASSERT_EQUALS(m_model->getDatasetDomainNames(), datasetNames);
  }

  void test_that_getDatasetNames_returns_the_expected_workspace_names_for_multi_spectra_workspaces() {
    auto const datasets = createDatasets(QStringList({"WSName1", "WSName2"}), "0,2-3");

    m_model->setNumberDomains(6);
    m_model->setDatasets(datasets);

    auto const expectedNames = QStringList({"WSName1", "WSName1", "WSName1", "WSName2", "WSName2", "WSName2"});
    TS_ASSERT_EQUALS(m_model->getDatasetNames().size(), 6);
    TS_ASSERT_EQUALS(m_model->getDatasetNames(), expectedNames);
  }

  void test_that_getDatasetDomainNames_returns_the_expected_domain_names_for_multi_spectra_workspaces() {
    auto const datasets = createDatasets(QStringList({"WSName1", "WSName2"}), "0,2-3");

    m_model->setNumberDomains(6);
    m_model->setDatasets(datasets);

    auto const expectedNames =
        QStringList({"WSName1 (0)", "WSName1 (2)", "WSName1 (3)", "WSName2 (0)", "WSName2 (2)", "WSName2 (3)"});
    TS_ASSERT_EQUALS(m_model->getDatasetDomainNames(), expectedNames);
  }

  void test_that_getDatasetNames_and_getDatasetDomainNames_returns_the_same_number_of_names() {
    auto const datasets = createDatasets(QStringList({"WSName1", "WSName2"}), "0,2-3");

    m_model->setNumberDomains(6);
    m_model->setDatasets(datasets);

    /// This is essential for EditLocalParameterDialog to find the sample logs
    /// of each dataset
    TS_ASSERT_EQUALS(m_model->getDatasetNames().size(), m_model->getDatasetDomainNames().size());
  }

private:
  std::unique_ptr<FunctionModel> m_model;
};
