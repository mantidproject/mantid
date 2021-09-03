// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../Reduction/MockBatch.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "PreviewJobManager.h"

#include "test/Batch/BatchJobManagerTest.h"
#include "test/Batch/MockReflAlgorithmFactory.h"
#include "test/ReflMockObjects.h"

#include <gmock/gmock.h>

using ::testing::Eq;
using ::testing::Return;

class PreviewJobManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PreviewJobManagerTest *createSuite() { return new PreviewJobManagerTest(); }
  static void destroySuite(PreviewJobManagerTest *suite) { delete suite; }

  void testGetPreprocessingAlgorithm() {
    auto mockAlgFactory = std::make_unique<MockReflAlgorithmFactory>();
    auto mockJobRunner = MockJobRunner();
    auto previewRow = createPreviewRow();
    auto stubAlg = createConfiguredAlgorithm();
    EXPECT_CALL(*mockAlgFactory, makePreprocessingAlgorithm(Eq(previewRow))).Times(1).WillOnce(Return(stubAlg));
    // TODO test calls on mockJobRunner

    auto batch = MockBatch();
    auto jobManager = createJobManager(&mockJobRunner, std::move(mockAlgFactory));
    auto const preprocessAlg = jobManager.getPreprocessingAlgorithm(previewRow);
    TS_ASSERT_EQUALS(stubAlg, preprocessAlg);
  }

private:
  PreviewJobManager createJobManager(MockJobRunner *jobRunner, std::unique_ptr<IReflAlgorithmFactory> algFactory) {
    return PreviewJobManager(jobRunner, std::move(algFactory));
  }

  PreviewRow createPreviewRow() { return PreviewRow({"12345"}); }

  IConfiguredAlgorithm_sptr createConfiguredAlgorithm() {
    IAlgorithm_sptr stubAlg = std::make_shared<WorkspaceCreationHelper::StubAlgorithm>();
    auto emptyProps = MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps();
    return std::make_shared<MantidQt::API::ConfiguredAlgorithm>(stubAlg, emptyProps);
  }
};
