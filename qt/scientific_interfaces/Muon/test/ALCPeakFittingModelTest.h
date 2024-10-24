// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"

#include "../Muon/ALCPeakFittingModel.h"

#include <QSignalSpy>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::API;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Points;
using ::testing::_;

class MockALCPeakFittingModelSubscriber : public IALCPeakFittingModelSubscriber {
public:
  MOCK_METHOD(void, dataChanged, (), (const, override));
  MOCK_METHOD(void, fittedPeaksChanged, (), (const, override));
  MOCK_METHOD(void, errorInModel, (std::string const &), (const, override));
};

class ALCPeakFittingModelTest : public CxxTest::TestSuite {
  MockAlgorithmRunner *m_algorithmManager;
  std::unique_ptr<ALCPeakFittingModel> m_model;
  std::unique_ptr<MockALCPeakFittingModelSubscriber> m_subscriber;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingModelTest *createSuite() { return new ALCPeakFittingModelTest(); }
  static void destroySuite(ALCPeakFittingModelTest *suite) { delete suite; }

  ALCPeakFittingModelTest() {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override {
    auto algorithmRunner = std::make_unique<MockAlgorithmRunner>();
    m_algorithmManager = algorithmRunner.get();

    m_model = std::make_unique<ALCPeakFittingModel>(std::move(algorithmRunner));
    m_subscriber = std::make_unique<MockALCPeakFittingModelSubscriber>();
    m_model->subscribe(m_subscriber.get());
  }

  void tearDown() override {}

  void test_setData() {
    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    EXPECT_CALL(*m_subscriber, dataChanged()).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_model->setData(data));

    TS_ASSERT_EQUALS(m_model->data(), data);
  }

  void test_notifyBatchComplete() {
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    auto func = FunctionFactory::Instance().createInitialized("name=FlatBackground");
    ITableWorkspace_sptr tableWs = WorkspaceFactory::Instance().createTable();
    IAlgorithm_sptr fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();
    fit->setProperty("Function", func);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CreateOutput", true);
    fit->setProperty("OutputCompositeMembers", true);
    fit->execute();
    MatrixWorkspace_sptr outputWs = fit->getProperty("OutputWorkspace");
    ITableWorkspace_sptr outputParamsWs = fit->getProperty("OutputParameters");
    auto runtimeProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
    MantidQt::API::IConfiguredAlgorithm_sptr fitAlg =
        std::make_shared<MantidQt::API::ConfiguredAlgorithm>(fit, std::move(runtimeProps));

    EXPECT_CALL(*m_subscriber, fittedPeaksChanged()).Times(1);

    m_model->notifyBatchComplete(fitAlg, false);

    TS_ASSERT_EQUALS(m_model->data(), outputWs);
    TS_ASSERT_EQUALS(m_model->parameterTable(), outputParamsWs);
    TS_ASSERT_EQUALS(m_model->fittedPeaks(), func);
  }

  void test_fitPeaks_calls_the_execute_algorithm_runner_method() {
    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 8, 8);

    data->setHistogram(0, Points{1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00},
                       Counts{0.00, 0.01, 0.02, 0.37, 1.00, 0.37, 0.01, 0.00}, CountStandardDeviations(8, 0));

    m_model->setData(data);

    IFunction_const_sptr func = FunctionFactory::Instance().createInitialized("name=FlatBackground");

    EXPECT_CALL(*m_algorithmManager, execute(testing::An<IConfiguredAlgorithm_sptr>())).Times(1);

    TS_ASSERT_THROWS_NOTHING(m_model->fitPeaks(func));
  }

  void test_exportWorkspace() { TS_ASSERT_THROWS_NOTHING(m_model->exportWorkspace()); }
};
