// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_

#include "../../../ISISReflectometry/GUI/Batch/BatchJobRunner.h"
#include "../ModelCreationHelpers.h"
#include "../ReflMockObjects.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using Mantid::API::Workspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using MantidQt::API::IConfiguredAlgorithm;
using MantidQt::API::IConfiguredAlgorithm_sptr;
using WorkspaceCreationHelper::MockAlgorithm;

using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class BatchJobRunnerTest {
public:
  // The boilerplate methods are not included because this is just a base
  // class

  BatchJobRunnerTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"},
        m_tolerance(0.1), m_experiment(makeEmptyExperiment()),
        m_instrument(makeEmptyInstrument()),
        m_runsTable(m_instruments, m_tolerance, ReductionJobs()), m_slicing() {
    m_jobAlgorithm = boost::make_shared<MockBatchJobAlgorithm>();
  }

protected:
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
  boost::shared_ptr<MockBatchJobAlgorithm> m_jobAlgorithm;

  class BatchJobRunnerFriend : public BatchJobRunner {
    friend class BatchJobRunnerTest;
    friend class BatchJobRunnerProcessingTest;
    friend class BatchJobRunnerProgressBarTest;
    friend class BatchJobRunnerWorkspacesTest;

  public:
    BatchJobRunnerFriend(Batch batch) : BatchJobRunner(batch) {}
  };

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobAlgorithm));
  }

  RunsTable makeRunsTable(ReductionJobs reductionJobs) {
    return RunsTable(m_instruments, m_tolerance, std::move(reductionJobs));
  }

  BatchJobRunnerFriend
  makeJobRunner(ReductionJobs reductionJobs = ReductionJobs()) {
    m_experiment = makeEmptyExperiment();
    m_instrument = makeEmptyInstrument();
    m_runsTable = makeRunsTable(std::move(reductionJobs));
    m_slicing = Slicing();
    return BatchJobRunnerFriend(
        Batch(m_experiment, m_instrument, m_runsTable, m_slicing));
  }

  Workspace2D_sptr createWorkspace() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    return ws;
  }

  Row *getRow(BatchJobRunnerFriend &jobRunner, int groupIndex, int rowIndex) {
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto *row = &reductionJobs.mutableGroups()[groupIndex]
                     .mutableRows()[rowIndex]
                     .get();
    return row;
  }

  Group &getGroup(BatchJobRunnerFriend &jobRunner, int groupIndex) {
    auto &reductionJobs =
        jobRunner.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[groupIndex];
    return group;
  }

  void selectGroup(BatchJobRunnerFriend &jobRunner, int groupIndex) {
    jobRunner.m_rowLocationsToProcess.push_back(
        MantidQt::MantidWidgets::Batch::RowPath{groupIndex});
  }

  void selectRow(BatchJobRunnerFriend &jobRunner, int groupIndex,
                 int rowIndex) {
    jobRunner.m_rowLocationsToProcess.push_back(
        MantidQt::MantidWidgets::Batch::RowPath{groupIndex, rowIndex});
  }
};

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNERTEST_H_
