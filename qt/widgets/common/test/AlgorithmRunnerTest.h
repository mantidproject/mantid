// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"
#include "MantidQtWidgets/Common/MockJobRunner.h"

using namespace MantidQt::API;
using namespace testing;

namespace {

IConfiguredAlgorithm_sptr createConfiguredAlgorithm() {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  auto alg = Mantid::API::AlgorithmManager::Instance().create("Rebin");
  IConfiguredAlgorithm_sptr configuredAlg =
      std::make_shared<MantidQt::API::ConfiguredAlgorithm>(std::move(alg), std::move(properties));
  return configuredAlg;
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockAlgorithmRunnerSubscriber : public IAlgorithmRunnerSubscriber {

public:
  virtual ~MockAlgorithmRunnerSubscriber() = default;

  MOCK_METHOD2(notifyBatchComplete, void(IConfiguredAlgorithm_sptr &lastAlgorithm, bool error));
  MOCK_METHOD0(notifyBatchCancelled, void());
  MOCK_METHOD1(notifyAlgorithmStarted, void(IConfiguredAlgorithm_sptr &algorithm));
  MOCK_METHOD1(notifyAlgorithmComplete, void(IConfiguredAlgorithm_sptr &algorithm));
  MOCK_METHOD2(notifyAlgorithmError, void(IConfiguredAlgorithm_sptr &algorithm, std::string const &message));
};

MATCHER_P(CheckAlgorithmName, name, "Check the algorithm's name") { return arg->algorithm()->name() == name; }
MATCHER(CheckAlgorithmNull, "Check the algorithm is a nullptr") { return !arg; }
MATCHER_P(CheckQueueSize, size, "Check the size of a std::deque") { return arg.size() == size; }

GNU_DIAG_ON_SUGGEST_OVERRIDE

class AlgorithmRunnerTest : public CxxTest::TestSuite {
public:
  static AlgorithmRunnerTest *createSuite() { return new AlgorithmRunnerTest; }
  static void destroySuite(AlgorithmRunnerTest *suite) { delete suite; }

  void setUp() override {
    m_configuredAlg = createConfiguredAlgorithm();

    auto jobRunner = std::make_unique<NiceMock<MockJobRunner>>();
    m_jobRunner = jobRunner.get();
    m_algorithmRunner = std::make_unique<AlgorithmRunner>(std::move(jobRunner));

    m_subscriber = std::make_unique<NiceMock<MockAlgorithmRunnerSubscriber>>();
    m_algorithmRunner->subscribe(m_subscriber.get());
  }

  void test_execute_calls_the_expected_job_runner_methods() {
    EXPECT_CALL(*m_jobRunner, executeAlgorithm(CheckAlgorithmName("Rebin"))).Times(1);
    m_algorithmRunner->execute(m_configuredAlg);
  }

  void test_execute_queue_calls_the_expected_job_runner_methods() {
    auto algorithmQueue = std::deque{m_configuredAlg, m_configuredAlg};

    EXPECT_CALL(*m_jobRunner, setAlgorithmQueue(CheckQueueSize(algorithmQueue.size()))).Times(1);
    EXPECT_CALL(*m_jobRunner, executeAlgorithmQueue()).Times(1);

    m_algorithmRunner->execute(algorithmQueue);
  }

  void test_notifyAlgorithmError_will_notify_the_subscriber() {
    std::string const errorMessage("Error message");

    EXPECT_CALL(*m_subscriber, notifyAlgorithmError(CheckAlgorithmName("Rebin"), errorMessage)).Times(1);

    m_algorithmRunner->notifyAlgorithmError(m_configuredAlg, errorMessage);
  }

  void test_notifyAlgorithmComplete_will_notify_the_subscriber() {
    EXPECT_CALL(*m_subscriber, notifyAlgorithmComplete(CheckAlgorithmName("Rebin"))).Times(1);
    m_algorithmRunner->notifyAlgorithmComplete(m_configuredAlg);
  }

  void test_notifyAlgorithmStarted_will_notify_the_subscriber() {
    EXPECT_CALL(*m_subscriber, notifyAlgorithmStarted(CheckAlgorithmName("Rebin"))).Times(1);
    m_algorithmRunner->notifyAlgorithmStarted(m_configuredAlg);
  }

  void test_notifyBatchCancelled_will_notify_the_subscriber() {
    EXPECT_CALL(*m_subscriber, notifyBatchCancelled()).Times(1);
    m_algorithmRunner->notifyBatchCancelled();
  }

  void test_notifyBatchComplete_will_notify_the_subscriber_with_a_nullptr_if_no_previously_run_algorithm() {
    bool const error(true);

    EXPECT_CALL(*m_subscriber, notifyBatchComplete(CheckAlgorithmNull(), error)).Times(1);

    m_algorithmRunner->notifyBatchComplete(error);
  }

  void test_notifyBatchComplete_will_notify_the_subscriber_with_an_algorithm_if_it_has_a_previously_run_algorithm() {
    bool const error(true);

    EXPECT_CALL(*m_subscriber, notifyAlgorithmComplete(CheckAlgorithmName("Rebin"))).Times(1);
    m_algorithmRunner->notifyAlgorithmComplete(m_configuredAlg);
    EXPECT_CALL(*m_subscriber, notifyBatchComplete(CheckAlgorithmName("Rebin"), error)).Times(1);

    m_algorithmRunner->notifyBatchComplete(error);
  }

private:
  IConfiguredAlgorithm_sptr m_configuredAlg;

  NiceMock<MockJobRunner> *m_jobRunner;
  std::unique_ptr<NiceMock<MockAlgorithmRunnerSubscriber>> m_subscriber;
  std::unique_ptr<AlgorithmRunner> m_algorithmRunner;
};
