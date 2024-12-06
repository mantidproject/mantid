// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>

#include "BayesFitting/StretchModel.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

class BayesStretch : public Algorithm {
public:
  const std::string name() const override { return "BayesStretch"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "BayesStretch Mock algorithm"; };

private:
  void init() override {
    declareProperty("SampleWorkspace", "");
    declareProperty("ResolutionWorkspace", "");
    declareProperty("EMin", 0.0);
    declareProperty("EMax", 0.0);
    declareProperty("NumberBeta", 0);
    declareProperty("Elastic", false);
    declareProperty("OutputWorkspaceFit", "");
    declareProperty("OutputWorkspaceContour", "");
    declareProperty("Background", "");
    declareProperty("SampleBins", 0);
    declareProperty("NumberSigma", 0);
    declareProperty("Loop", false);
  };

  void exec() override {
    ITableWorkspace_sptr outputWS = WorkspaceFactory::Instance().createTable();

    outputWS->addColumn("str", "SampleWorkspace");
    outputWS->addColumn("str", "ResolutionWorkspace");
    outputWS->addColumn("double", "EMin");
    outputWS->addColumn("double", "EMax");
    outputWS->addColumn("int", "NumberBeta");
    outputWS->addColumn("bool", "Elastic");
    outputWS->addColumn("str", "OutputWorkspaceFit");
    outputWS->addColumn("str", "OutputWorkspaceContour");
    outputWS->addColumn("str", "Background");
    outputWS->addColumn("int", "SampleBins");
    outputWS->addColumn("int", "NumberSigma");
    outputWS->addColumn("bool", "Loop");

    TableRow newRow = outputWS->appendRow();

    auto sampleWS = getPropertyValue("SampleWorkspace");
    auto resolutionWS = getPropertyValue("ResolutionWorkspace");
    auto eMin = std::stod(getPropertyValue("EMin"));
    auto eMax = std::stod(getPropertyValue("EMax"));
    auto numBeta = std::stoi(getPropertyValue("NumberBeta"));
    auto elastic = getPropertyValue("Elastic") == "1";
    auto outputFit = getPropertyValue("OutputWorkspaceFit");
    auto outputContour = getPropertyValue("OutputWorkspaceContour");
    auto background = getPropertyValue("Background");
    auto sampleBins = std::stoi(getPropertyValue("SampleBins"));
    auto numSigma = std::stoi(getPropertyValue("NumberSigma"));
    auto loop = getPropertyValue("Loop") == "1";

    newRow << sampleWS << resolutionWS << eMin << eMax << numBeta << elastic << outputFit << outputContour << background
           << sampleBins << numSigma << loop;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};

class BayesStretch2 : public Algorithm {
public:
  const std::string name() const override { return "BayesStretch2"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "BayesStretch2 Mock algorithm"; };

private:
  void init() override {
    declareProperty("SampleWorkspace", "");
    declareProperty("ResolutionWorkspace", "");
    declareProperty("EMin", 0.0);
    declareProperty("EMax", 0.0);
    declareProperty("NumberBeta", 0);
    declareProperty("Elastic", false);
    declareProperty("OutputWorkspaceFit", "");
    declareProperty("OutputWorkspaceContour", "");
    declareProperty("Background", "");
  };

  void exec() override {
    ITableWorkspace_sptr outputWS = WorkspaceFactory::Instance().createTable();

    outputWS->addColumn("str", "SampleWorkspace");
    outputWS->addColumn("str", "ResolutionWorkspace");
    outputWS->addColumn("double", "EMin");
    outputWS->addColumn("double", "EMax");
    outputWS->addColumn("int", "NumberBeta");
    outputWS->addColumn("bool", "Elastic");
    outputWS->addColumn("str", "OutputWorkspaceFit");
    outputWS->addColumn("str", "OutputWorkspaceContour");
    outputWS->addColumn("str", "Background");

    TableRow newRow = outputWS->appendRow();

    auto sampleWS = getPropertyValue("SampleWorkspace");
    auto resolutionWS = getPropertyValue("ResolutionWorkspace");
    auto eMin = std::stod(getPropertyValue("EMin"));
    auto eMax = std::stod(getPropertyValue("EMax"));
    auto numBeta = std::stoi(getPropertyValue("NumberBeta"));
    auto elastic = getPropertyValue("Elastic") == "1";
    auto outputFit = getPropertyValue("OutputWorkspaceFit");
    auto outputContour = getPropertyValue("OutputWorkspaceContour");
    auto background = getPropertyValue("Background");

    newRow << sampleWS << resolutionWS << eMin << eMax << numBeta << elastic << outputFit << outputContour
           << background;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};

DECLARE_ALGORITHM(BayesStretch)
DECLARE_ALGORITHM(BayesStretch2)

class StretchModelTest : public CxxTest::TestSuite {
public:
  static StretchModelTest *createSuite() { return new StretchModelTest(); }
  static void destroySuite(StretchModelTest *suite) { delete suite; }

  void setUp() override {
    m_model = std::make_unique<StretchModel>();
    Mantid::API::FrameworkManager::Instance();
  }

  void test_stretchAlgorithm_creates_BayesStretch_by_default() {
    StretchRunData params("sample_ws", "res_ws", -0.5, 0.5, 50, true, "flat", 30, 1, true);

    auto configuredAlgorithm = m_model->stretchAlgorithm(params, "fit_ws", "contour_ws");

    TS_ASSERT_EQUALS("BayesStretch", configuredAlgorithm->algorithm()->name());

    auto &properties = configuredAlgorithm->getAlgorithmRuntimeProps();

    TS_ASSERT_EQUALS("sample_ws", properties.getPropertyValue("SampleWorkspace"));
    TS_ASSERT_EQUALS("res_ws", properties.getPropertyValue("ResolutionWorkspace"));
    TS_ASSERT_EQUALS("-0.5", properties.getPropertyValue("EMin"));
    TS_ASSERT_EQUALS("0.5", properties.getPropertyValue("EMax"));
    TS_ASSERT_EQUALS("50", properties.getPropertyValue("NumberBeta"));
    TS_ASSERT_EQUALS("30", properties.getPropertyValue("NumberSigma"));
    TS_ASSERT_EQUALS("1", properties.getPropertyValue("Elastic"));
    TS_ASSERT_EQUALS("fit_ws", properties.getPropertyValue("OutputWorkspaceFit"));
    TS_ASSERT_EQUALS("contour_ws", properties.getPropertyValue("OutputWorkspaceContour"));
    TS_ASSERT_EQUALS("flat", properties.getPropertyValue("Background"));
    TS_ASSERT_EQUALS("1", properties.getPropertyValue("SampleBins"));
    TS_ASSERT_EQUALS("1", properties.getPropertyValue("Loop"));
  }

  void test_stretchAlgorithm_creates_BayesStretch2_when_quickbayes_enabled() {
    StretchRunData params("sample_ws", "res_ws", -0.5, 0.5, 50, true, "flat", 30, 1, true);

    auto configuredAlgorithm = m_model->stretchAlgorithm(params, "fit_ws", "contour_ws", true);

    TS_ASSERT_EQUALS("BayesStretch2", configuredAlgorithm->algorithm()->name());

    auto &properties = configuredAlgorithm->getAlgorithmRuntimeProps();

    TS_ASSERT_EQUALS("sample_ws", properties.getPropertyValue("SampleWorkspace"));
    TS_ASSERT_EQUALS("res_ws", properties.getPropertyValue("ResolutionWorkspace"));
    TS_ASSERT_EQUALS("-0.5", properties.getPropertyValue("EMin"));
    TS_ASSERT_EQUALS("0.5", properties.getPropertyValue("EMax"));
    TS_ASSERT_EQUALS("50", properties.getPropertyValue("NumberBeta"));
    TS_ASSERT_EQUALS("1", properties.getPropertyValue("Elastic"));
    TS_ASSERT_EQUALS("fit_ws", properties.getPropertyValue("OutputWorkspaceFit"));
    TS_ASSERT_EQUALS("contour_ws", properties.getPropertyValue("OutputWorkspaceContour"));
    TS_ASSERT_EQUALS("flat", properties.getPropertyValue("Background"));

    TS_ASSERT(!properties.existsProperty("SampleBins"));
    TS_ASSERT(!properties.existsProperty("NumberSigma"));
    TS_ASSERT(!properties.existsProperty("Loop"));
  }

  void test_setupSaveAlgorithm_creates_correct_save_algorithm() {
    const std::string wsName = "test_workspace";
    auto configuredAlgorithm = m_model->setupSaveAlgorithm(wsName);

    auto &properties = configuredAlgorithm->getAlgorithmRuntimeProps();

    TS_ASSERT_EQUALS("SaveNexusProcessed", configuredAlgorithm->algorithm()->name());
    TS_ASSERT_EQUALS("test_workspace.nxs", properties.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS("test_workspace", properties.getPropertyValue("InputWorkspace"));
  }

private:
  std::unique_ptr<StretchModel> m_model;
};
