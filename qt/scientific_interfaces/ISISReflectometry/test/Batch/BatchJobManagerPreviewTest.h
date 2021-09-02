// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BatchJobManagerTest.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockReflAlgorithmFactory.h"

#include <gmock/gmock.h>

using ::testing::Eq;
using ::testing::Return;

class BatchJobManagerPreviewTest : public CxxTest::TestSuite, public BatchJobManagerTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchJobManagerPreviewTest *createSuite() { return new BatchJobManagerPreviewTest(); }
  static void destroySuite(BatchJobManagerPreviewTest *suite) { delete suite; }

  void testGetPreprocessingAlgorithm() {
    auto mock = std::make_unique<MockReflAlgorithmFactory>();
    auto previewRow = createPreviewRow();
    auto stubAlg = createConfiguredAlgorithm();
    EXPECT_CALL(*mock, makePreprocessingAlgorithm(Eq(previewRow))).Times(1).WillOnce(Return(stubAlg));

    auto jobManager = makeJobManager(std::move(mock));
    auto const preprocessAlg = jobManager.getPreprocessingAlgorithm(previewRow);
    TS_ASSERT_EQUALS(stubAlg, preprocessAlg);
  }

private:
  PreviewRow createPreviewRow() { return PreviewRow({"12345"}); }

  IConfiguredAlgorithm_sptr createConfiguredAlgorithm() {
    IAlgorithm_sptr stubAlg = std::make_shared<WorkspaceCreationHelper::StubAlgorithm>();
    auto emptyProps = MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps();
    return std::make_shared<MantidQt::API::ConfiguredAlgorithm>(stubAlg, emptyProps);
  }
};
