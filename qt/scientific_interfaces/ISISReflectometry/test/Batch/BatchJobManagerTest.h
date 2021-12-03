// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Batch/BatchJobManager.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../ReflMockObjects.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using Mantid::API::Workspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using MantidQt::API::IConfiguredAlgorithm;
using MantidQt::API::IConfiguredAlgorithm_sptr;

using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class BatchJobManagerTest {
public:
  // The boilerplate methods are not included because this is just a base
  // class

  BatchJobManagerTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"}, m_tolerance(0.1),
        m_experiment(makeEmptyExperiment()), m_instrument(makeEmptyInstrument()),
        m_runsTable(m_instruments, m_tolerance, ReductionJobs()), m_slicing(),
        m_batch(m_experiment, m_instrument, m_runsTable, m_slicing) {
    m_jobAlgorithm = std::make_shared<MockBatchJobAlgorithm>();
  }

protected:
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
  Batch m_batch;
  std::shared_ptr<MockBatchJobAlgorithm> m_jobAlgorithm;

  class BatchJobManagerFriend : public BatchJobManager {
    friend class BatchJobManagerTest;
    friend class BatchJobManagerProcessingTest;
    friend class BatchJobManagerProgressBarTest;
    friend class BatchJobManagerWorkspacesTest;

  public:
    BatchJobManagerFriend(Batch &batch, std::unique_ptr<IReflAlgorithmFactory> factory)
        : BatchJobManager(batch, std::move(factory)) {}
  };

  void verifyAndClear() { TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobAlgorithm)); }

  RunsTable makeRunsTable(ReductionJobs reductionJobs) {
    return RunsTable(m_instruments, m_tolerance, std::move(reductionJobs));
  }

  BatchJobManagerFriend makeJobManager(ReductionJobs reductionJobs = ReductionJobs()) {
    return makeJobManager(reductionJobs, nullptr);
  }

  BatchJobManagerFriend makeJobManager(std::unique_ptr<IReflAlgorithmFactory> mockFactory) {
    return makeJobManager(ReductionJobs(), std::move(mockFactory));
  }

  BatchJobManagerFriend makeJobManager(ReductionJobs reductionJobs,
                                       std::unique_ptr<IReflAlgorithmFactory> mockFactory) {
    m_runsTable = makeRunsTable(std::move(reductionJobs));
    return BatchJobManagerFriend(m_batch, std::move(mockFactory));
  }

  Workspace2D_sptr createWorkspace() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    return ws;
  }

  Row *getRow(BatchJobManagerFriend &jobManager, int groupIndex, int rowIndex) {
    auto &reductionJobs = jobManager.m_batch.mutableRunsTable().mutableReductionJobs();
    auto *row = &reductionJobs.mutableGroups()[groupIndex].mutableRows()[rowIndex].get();
    return row;
  }

  Group &getGroup(BatchJobManagerFriend &jobManager, int groupIndex) {
    auto &reductionJobs = jobManager.m_batch.mutableRunsTable().mutableReductionJobs();
    auto &group = reductionJobs.mutableGroups()[groupIndex];
    return group;
  }

  void selectGroup(BatchJobManagerFriend &jobManager, int groupIndex) {
    jobManager.m_rowLocationsToProcess.push_back(MantidQt::MantidWidgets::Batch::RowPath{groupIndex});
    auto selectedRowLocation = MantidQt::MantidWidgets::Batch::RowPath{groupIndex};
    jobManager.m_batch.mutableRunsTable().appendSelectedRowLocations(std::move(selectedRowLocation));
  }

  void selectRow(BatchJobManagerFriend &jobManager, int groupIndex, int rowIndex) {
    jobManager.m_rowLocationsToProcess.push_back(MantidQt::MantidWidgets::Batch::RowPath{groupIndex, rowIndex});
    auto selectedPath = MantidQt::MantidWidgets::Batch::RowPath{groupIndex, rowIndex};
    jobManager.m_batch.mutableRunsTable().appendSelectedRowLocations({std::move(selectedPath)});
  }
};
