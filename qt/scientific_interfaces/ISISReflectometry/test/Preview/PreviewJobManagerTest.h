// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../Reduction/MockBatch.h"
#include "GUI/Batch/BatchJobAlgorithm.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "PreviewJobManager.h"

#include "test/Batch/BatchJobManagerTest.h"
#include "test/Batch/MockReflAlgorithmFactory.h"
#include "test/ReflMockObjects.h"

#include <gmock/gmock.h>

using ::testing::ByRef;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Return;

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

  void test_start_preprocessing() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    auto mockJobRunner = MockJobRunner();
    auto previewRow = createPreviewRow();
    auto stubAlg = makeConfiguredAlg();
    EXPECT_CALL(*mockAlgFactory, makePreprocessingAlgorithm(Eq(ByRef(previewRow)))).Times(1).WillOnce(Return(stubAlg));
    EXPECT_CALL(mockJobRunner, clearAlgorithmQueue()).Times(1);
    EXPECT_CALL(mockJobRunner, setAlgorithmQueue(Eq(std::deque<IConfiguredAlgorithm_sptr>{stubAlg}))).Times(1);
    EXPECT_CALL(mockJobRunner, executeAlgorithmQueue()).Times(1);

    auto batch = MockBatch();
    auto jobManager = createJobManager(&mockJobRunner, std::move(mockAlgFactory));
    jobManager.startPreprocessing(previewRow);
  }

  void test_subscribe_to_job_runner() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    auto mockJobRunner = MockJobRunner();
    EXPECT_CALL(mockJobRunner, subscribe(NotNull())).Times(1);
    auto jobManager = PreviewJobManager(&mockJobRunner, std::move(mockAlgFactory));
  }

  void test_notify_preprocessing_algorithm_complete_notifies_subscriber() {
    AlgCompleteCallback::m_callbackWasCalled = false;

    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    auto mockJobRunner = MockJobRunner();
    auto mockSubscriber = MockJobManagerSubscriber();

    auto row = PreviewRow({"12345"});
    Mantid::API::IAlgorithm_sptr mockAlg = std::make_shared<WorkspaceCreationHelper::StubAlgorithm>();
    auto properties = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
    auto configuredAlg = std::make_shared<BatchJobAlgorithm>(std::move(mockAlg), std::move(properties),
                                                             AlgCompleteCallback::updateRowOnAlgorithmComplete, &row);

    EXPECT_CALL(mockSubscriber, notifyLoadWorkspaceCompleted).Times(1);
    auto jobManager = PreviewJobManager(&mockJobRunner, std::move(mockAlgFactory));
    jobManager.subscribe(&mockSubscriber);

    auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);
    jobManager.notifyAlgorithmComplete(configuredAlgRef);
    TS_ASSERT(AlgCompleteCallback::m_callbackWasCalled);
  }

  void test_notify_preprocessing_algorithm_complete_skips_non_preview_items() {
    // Other item types are Row and Group
    auto row = makeEmptyRow();
    auto group = makeEmptyGroup();

    auto items = std::array<Item *, 2>{&row, &group};

    for (auto *item : items) {
      auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
      auto mockJobRunner = MockJobRunner();
      auto mockSubscriber = MockJobManagerSubscriber();

      auto configuredAlg = makeConfiguredAlg(*item);

      EXPECT_CALL(mockSubscriber, notifyLoadWorkspaceCompleted).Times(0);

      auto jobManager = PreviewJobManager(&mockJobRunner, std::move(mockAlgFactory));
      jobManager.subscribe(&mockSubscriber);
      auto configuredAlgRef = std::static_pointer_cast<IConfiguredAlgorithm>(configuredAlg);
      jobManager.notifyAlgorithmComplete(configuredAlgRef);
    }
  }

private:
  PreviewJobManager createJobManager(MockJobRunner *jobRunner, std::unique_ptr<IReflAlgorithmFactory> algFactory) {
    return PreviewJobManager(jobRunner, std::move(algFactory));
  }

  PreviewRow createPreviewRow() { return PreviewRow({"12345"}); }

  IConfiguredAlgorithm_sptr makeConfiguredAlg() {
    IAlgorithm_sptr stubAlg = std::make_shared<WorkspaceCreationHelper::StubAlgorithm>();
    auto emptyProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
    return std::make_shared<MantidQt::API::ConfiguredAlgorithm>(std::move(stubAlg), std::move(emptyProps));
  }

  std::shared_ptr<BatchJobAlgorithm> makeConfiguredAlg(Item &item) {
    Mantid::API::IAlgorithm_sptr mockAlg = std::make_shared<WorkspaceCreationHelper::StubAlgorithm>();
    auto properties = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
    auto configuredAlg = std::make_shared<BatchJobAlgorithm>(std::move(mockAlg), std::move(properties), nullptr, &item);
    return configuredAlg;
  }
};
