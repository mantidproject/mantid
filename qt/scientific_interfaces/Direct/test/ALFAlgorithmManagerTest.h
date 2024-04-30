// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAlgorithmManager.h"
#include "MockALFAlgorithmManagerSubscriber.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/MockJobRunner.h"

#include <memory>

using namespace Mantid::API;
using namespace testing;

namespace {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

MATCHER_P(CheckAlgorithmName, name, "Check the algorithm's name") { return arg->algorithm()->name() == name; }

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace

class ALFAlgorithmManagerTest : public CxxTest::TestSuite {
public:
  ALFAlgorithmManagerTest() { FrameworkManager::Instance(); }

  static ALFAlgorithmManagerTest *createSuite() { return new ALFAlgorithmManagerTest(); }

  static void destroySuite(ALFAlgorithmManagerTest *suite) { delete suite; }

  void setUp() override {
    m_algProperties = std::make_unique<AlgorithmRuntimeProps>();

    auto jobRunner = std::make_unique<NiceMock<MockJobRunner>>();
    m_jobRunner = jobRunner.get();
    m_algorithmManager = std::make_unique<MantidQt::CustomInterfaces::ALFAlgorithmManager>(std::move(jobRunner));

    m_subscriber = std::make_unique<NiceMock<MantidQt::CustomInterfaces::MockALFAlgorithmManagerSubscriber>>();
    m_algorithmManager->subscribe(m_subscriber.get());
  }

  void tearDown() override { m_algorithmManager.reset(); }

  void test_load_will_execute_the_load_algorithm() {
    expectExecuteAlgorithm("Load");
    m_algorithmManager->load(std::move(m_algProperties));
  }

  void test_normaliseByCurrent_will_execute_the_normalise_by_current_algorithm() {
    expectExecuteAlgorithm("NormaliseByCurrent");
    m_algorithmManager->normaliseByCurrent(std::move(m_algProperties));
  }

  void test_rebinToWorkspace_will_execute_the_rebin_to_workspace_algorithm() {
    expectExecuteAlgorithm("RebinToWorkspace");
    m_algorithmManager->rebinToWorkspace(std::move(m_algProperties));
  }

  void test_divide_will_execute_the_divide_algorithm() {
    expectExecuteAlgorithm("Divide");
    m_algorithmManager->divide(std::move(m_algProperties));
  }

  void test_replaceSpecialValues_will_execute_the_replace_special_values_algorithm() {
    expectExecuteAlgorithm("ReplaceSpecialValues");
    m_algorithmManager->replaceSpecialValues(std::move(m_algProperties));
  }

  void test_convertUnits_will_execute_the_convert_units_algorithm() {
    expectExecuteAlgorithm("ConvertUnits");
    m_algorithmManager->convertUnits(std::move(m_algProperties));
  }

  void test_createWorkspace_will_execute_the_create_workspace_units_algorithm() {
    expectExecuteAlgorithm("CreateWorkspace");
    m_algorithmManager->createWorkspace(std::move(m_algProperties));
  }

  void test_scaleX_will_execute_the_scale_x_algorithm() {
    expectExecuteAlgorithm("ScaleX");
    m_algorithmManager->scaleX(std::move(m_algProperties));
  }

  void test_rebunch_will_execute_the_rebunch_algorithm() {
    expectExecuteAlgorithm("Rebunch");
    m_algorithmManager->rebunch(std::move(m_algProperties));
  }

  void test_cropWorkspace_will_execute_the_crop_workspace_algorithm() {
    expectExecuteAlgorithm("CropWorkspace");
    m_algorithmManager->cropWorkspace(std::move(m_algProperties));
  }

  void test_fit_will_execute_the_fit_algorithm() {
    expectExecuteAlgorithm("Fit");

    Mantid::API::IFunction_sptr function = Mantid::API::FunctionFactory::Instance().createFunction("Gaussian");
    Mantid::API::Workspace_sptr workspace = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    Mantid::API::AlgorithmProperties::update("Function", function, *m_algProperties);
    Mantid::API::AlgorithmProperties::update("InputWorkspace", workspace, *m_algProperties);
    Mantid::API::AlgorithmProperties::update("CreateOutput", true, *m_algProperties);
    Mantid::API::AlgorithmProperties::update("StartX", -15.0, *m_algProperties);
    Mantid::API::AlgorithmProperties::update("EndX", 15.0, *m_algProperties);

    m_algorithmManager->fit(std::move(m_algProperties));
  }

  void test_notifyAlgorithmError_will_notify_the_subscriber() {
    std::string const errorMessage("Error message");
    auto alg = Mantid::API::AlgorithmManager::Instance().create("Rebin");
    MantidQt::API::IConfiguredAlgorithm_sptr configuredAlg =
        std::make_shared<MantidQt::API::ConfiguredAlgorithm>(std::move(alg), std::move(m_algProperties));

    EXPECT_CALL(*m_subscriber, notifyAlgorithmError(errorMessage)).Times(1);

    m_algorithmManager->notifyAlgorithmError(configuredAlg, errorMessage);
  }

private:
  void expectExecuteAlgorithm(std::string const &name) {
    EXPECT_CALL(*m_jobRunner, executeAlgorithm(CheckAlgorithmName(name))).Times(1);
  }

  std::unique_ptr<AlgorithmRuntimeProps> m_algProperties;

  std::unique_ptr<NiceMock<MantidQt::CustomInterfaces::MockALFAlgorithmManagerSubscriber>> m_subscriber;
  NiceMock<MockJobRunner> *m_jobRunner;
  std::unique_ptr<MantidQt::CustomInterfaces::ALFAlgorithmManager> m_algorithmManager;
};
