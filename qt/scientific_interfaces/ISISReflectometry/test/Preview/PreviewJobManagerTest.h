// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/BatchJobAlgorithm.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "PreviewJobManager.h"

#include "test/Batch/BatchJobManagerTest.h"
#include "test/Batch/MockReflAlgorithmFactory.h"
#include "test/ReflMockObjects.h"

#include <gmock/gmock.h>

using Mantid::API::IAlgorithm_sptr;
using MantidQt::API::AlgorithmRuntimeProps;
using MantidQt::API::ConfiguredAlgorithm;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Return;
using WorkspaceCreationHelper::StubAlgorithm;

namespace {
struct AlgCompleteCallback {
  static void updateRowOnAlgorithmComplete(const IAlgorithm_sptr &, Item &) {
    AlgCompleteCallback::m_callbackWasCalled = true;
  }
  static bool m_callbackWasCalled;
};

bool AlgCompleteCallback::m_callbackWasCalled = false;
} // namespace

class PreviewJobManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PreviewJobManagerTest *createSuite() { return new PreviewJobManagerTest(); }
  static void destroySuite(PreviewJobManagerTest *suite) { delete suite; }

  void test_subscribe_to_job_runner() {
    auto mockJobRunner = MockJobRunner();
    auto jobManager = makeJobManager(&mockJobRunner);
  }

  void test_start_preprocessing() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    auto mockJobRunner = MockJobRunner();
    auto previewRow = makePreviewRow();
    auto stubAlg = makeConfiguredAlg();

    expectPreprocessingAlgCreated(*mockAlgFactory, previewRow, stubAlg);
    expectAlgorithmExecuted(stubAlg, mockJobRunner);

    auto jobManager = makeJobManager(&mockJobRunner, std::move(mockAlgFactory));
    jobManager.startPreprocessing(previewRow);
  }

  void test_notify_preprocessing_algorithm_complete_notifies_subscriber() {
    auto mockJobRunner = MockJobRunner();
    auto mockSubscriber = MockJobManagerSubscriber();
    auto previewRow = makePreviewRow();
    auto stubAlg = makeConfiguredPreprocessAlg(previewRow);

    EXPECT_CALL(mockSubscriber, notifyLoadWorkspaceCompleted).Times(1);

    auto jobManager = makeJobManager(&mockJobRunner, mockSubscriber);
    auto stubAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(stubAlg);
    jobManager.notifyAlgorithmComplete(stubAlgRef);

    assertUpdateItemCallbackWasCalled();
  }

  void test_notify_preprocessing_algorithm_complete_skips_non_preview_items() {
    // Other item types are Row and Group
    auto row = makeEmptyRow();
    auto group = makeEmptyGroup();

    auto items = std::array<Item *, 2>{&row, &group};

    for (auto *item : items) {
      auto mockJobRunner = MockJobRunner();
      auto mockSubscriber = MockJobManagerSubscriber();
      auto configuredAlg = makeConfiguredAlg(*item);

      EXPECT_CALL(mockSubscriber, notifyLoadWorkspaceCompleted).Times(0);

      auto jobManager = makeJobManager(&mockJobRunner, mockSubscriber);
      auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);
      jobManager.notifyAlgorithmComplete(configuredAlgRef);
    }
  }

  void test_start_sum_banks() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    auto mockJobRunner = MockJobRunner();
    auto previewRow = makePreviewRow();
    auto stubAlg = makeConfiguredAlg();

    expectSumBanksAlgorithmCreated(*mockAlgFactory, previewRow, stubAlg);
    expectAlgorithmExecuted(stubAlg, mockJobRunner);

    auto jobManager = makeJobManager(&mockJobRunner, std::move(mockAlgFactory));
    jobManager.startSumBanks(previewRow);
  }

  void test_notify_sum_banks_algorithm_complete_notifies_subscriber() {
    auto mockJobRunner = MockJobRunner();
    auto mockSubscriber = MockJobManagerSubscriber();

    auto previewRow = makePreviewRow();
    auto stubAlg = makeConfiguredSumBanksAlg(previewRow);

    EXPECT_CALL(mockSubscriber, notifySumBanksCompleted).Times(1);

    auto jobManager = makeJobManager(&mockJobRunner, mockSubscriber);
    auto stubAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(stubAlg);
    jobManager.notifyAlgorithmComplete(stubAlgRef);

    assertUpdateItemCallbackWasCalled();
  }

  void test_notify_algorithm_complete_throws_with_unknown_algorithm() {
    auto mockJobRunner = MockJobRunner();
    auto mockSubscriber = MockJobManagerSubscriber();
    auto previewRow = makePreviewRow();
    auto configuredAlg = makeConfiguredAlg(previewRow);

    auto jobManager = makeJobManager(&mockJobRunner, mockSubscriber);
    auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);

    TS_ASSERT_THROWS(jobManager.notifyAlgorithmComplete(configuredAlgRef), std::logic_error const &);
  }

  void test_notify_algorithm_error_throws_with_unknown_algorithm() {
    auto mockJobRunner = MockJobRunner();
    auto mockSubscriber = MockJobManagerSubscriber();
    auto previewRow = makePreviewRow();
    auto configuredAlg = makeConfiguredAlg(previewRow);

    auto jobManager = makeJobManager(&mockJobRunner, mockSubscriber);
    auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);

    TS_ASSERT_THROWS(jobManager.notifyAlgorithmError(configuredAlgRef, ""), std::logic_error const &);
  }

  void test_notify_algorithm_complete_catches_runtime_errors() {
    auto mockJobRunner = MockJobRunner();
    auto mockSubscriber = MockJobManagerSubscriber();
    auto row = makePreviewRow();
    auto configuredAlg = makeConfiguredAlg(row, makeStubAlg(), updateFuncThatThrowsExpectedError);

    EXPECT_CALL(mockSubscriber, notifyLoadWorkspaceCompleted).Times(0);

    auto jobManager = makeJobManager(&mockJobRunner);
    auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);

    jobManager.notifyAlgorithmComplete(configuredAlgRef);
  }

  void test_notify_algorithm_complete_does_not_catch_unexpected_errors() {
    auto mockJobRunner = MockJobRunner();
    auto row = makePreviewRow();
    auto configuredAlg = makeConfiguredAlg(row, makeStubAlg(), updateFuncThatThrowsUnexpectedError);
    auto jobManager = makeJobManager(&mockJobRunner);
    auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);

    TS_ASSERT_THROWS(jobManager.notifyAlgorithmComplete(configuredAlgRef), std::invalid_argument const &);
  }

private:
  class StubAlgPreprocess : public StubAlgorithm {
  public:
    const std::string name() const override { return "ReflectometryISISPreprocess"; }
  };

  class StubAlgSumBanks : public StubAlgorithm {
  public:
    const std::string name() const override { return "ReflectometryISISSumBanks"; }
  };

  PreviewJobManager
  makeJobManager(MockJobRunner *mockJobRunner,
                 std::unique_ptr<IReflAlgorithmFactory> algFactory = std::make_unique<MockReflAlgorithmFactory>()) {
    EXPECT_CALL(*mockJobRunner, subscribe(NotNull())).Times(1);
    return PreviewJobManager(mockJobRunner, std::move(algFactory));
  }

  PreviewJobManager makeJobManager(MockJobRunner *mockJobRunner, MockJobManagerSubscriber &mockSubscriber) {
    auto jobManager = makeJobManager(mockJobRunner);
    jobManager.subscribe(&mockSubscriber);
    return jobManager;
  }

  PreviewRow makePreviewRow() { return PreviewRow({"12345"}); }

  IAlgorithm_sptr makeStubAlg() { return std::make_shared<StubAlgorithm>(); }

  // Create a basic configured algorithm using a StubAlgorithm
  IConfiguredAlgorithm_sptr makeConfiguredAlg() {
    IAlgorithm_sptr stubAlg = std::make_shared<StubAlgorithm>();
    auto emptyProps = std::make_unique<AlgorithmRuntimeProps>();
    return std::make_shared<ConfiguredAlgorithm>(std::move(stubAlg), std::move(emptyProps));
  }

  // Create a configured batch job algorithm that has an item and callback function set up on it
  // Uses StubAlgorithm by default, or can take an override.
  std::shared_ptr<BatchJobAlgorithm>
  makeConfiguredAlg(Item &item, IAlgorithm_sptr mockAlg = std::make_shared<StubAlgorithm>(),
                    BatchJobAlgorithm::UpdateFunction updateFunc = AlgCompleteCallback::updateRowOnAlgorithmComplete) {
    AlgCompleteCallback::m_callbackWasCalled = false;
    auto properties = std::make_unique<AlgorithmRuntimeProps>();
    auto configuredAlg =
        std::make_shared<BatchJobAlgorithm>(std::move(mockAlg), std::move(properties), updateFunc, &item);
    return configuredAlg;
  }

  std::shared_ptr<ConfiguredAlgorithm> makeConfiguredPreprocessAlg(Item &item) {
    return makeConfiguredAlg(item, std::make_shared<StubAlgPreprocess>());
  }

  std::shared_ptr<ConfiguredAlgorithm> makeConfiguredSumBanksAlg(Item &item) {
    return makeConfiguredAlg(item, std::make_shared<StubAlgSumBanks>());
  }

  void expectPreprocessingAlgCreated(MockReflAlgorithmFactory &mockAlgFactory, PreviewRow &previewRow,
                                     IConfiguredAlgorithm_sptr const &alg) {
    EXPECT_CALL(mockAlgFactory, makePreprocessingAlgorithm(Eq(ByRef(previewRow)))).Times(1).WillOnce(Return(alg));
  }

  void expectSumBanksAlgorithmCreated(MockReflAlgorithmFactory &mockAlgFactory, PreviewRow &previewRow,
                                      IConfiguredAlgorithm_sptr const &alg) {
    EXPECT_CALL(mockAlgFactory, makeSumBanksAlgorithm(Eq(ByRef(previewRow)))).Times(1).WillOnce(Return(alg));
  }

  void expectAlgorithmExecuted(IConfiguredAlgorithm_sptr const &alg, MockJobRunner &mockJobRunner) {
    EXPECT_CALL(mockJobRunner, clearAlgorithmQueue()).Times(1);
    EXPECT_CALL(mockJobRunner, setAlgorithmQueue(Eq(std::deque<IConfiguredAlgorithm_sptr>{alg}))).Times(1);
    EXPECT_CALL(mockJobRunner, executeAlgorithmQueue()).Times(1);
  }

  // For a configured batch job algorithm with a callback function set on it, ensure that the callback was called
  void assertUpdateItemCallbackWasCalled() { TS_ASSERT(AlgCompleteCallback::m_callbackWasCalled); }

  // A fake callback function to pass to the configured job algorithm that throws an expected error type (RuntimeError)
  static void updateFuncThatThrowsExpectedError(const Mantid::API::IAlgorithm_sptr &, Item &) {
    throw std::runtime_error("Test error");
  }

  // A fake callback function to pass to the configured job algorithm that throws an expected error type (not a
  // RuntimeError)
  static void updateFuncThatThrowsUnexpectedError(const Mantid::API::IAlgorithm_sptr &, Item &) {
    throw std::invalid_argument("Test error");
  }
};
