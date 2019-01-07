// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobRunner.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

using Mantid::API::IAlgorithm;
using API::BatchAlgorithmRunner;
using AlgorithmRuntimeProps = std::map<std::string, std::string>;

namespace { // unnamed
std::string boolToString(bool value) { return value ? "1" : "0"; }

void updateProperty(std::string const &property, std::string const &value,
                    AlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties[property] = value;
}

void updateProperty(std::string const &property, double value,
                    AlgorithmRuntimeProps &properties) {
  updateProperty(property, std::to_string(value), properties);
}

void updateProperty(std::string const &property,
                    boost::optional<double> const &value,
                    AlgorithmRuntimeProps &properties) {
  if (value)
    updateProperty(property, value.get(), properties);
}

void updateWorkspacesNameProperties(AlgorithmRuntimeProps &properties,
                                    ReductionWorkspaces const &workspaces) {
  updateProperty("InputWorkspace", workspaces.joinedTofWorkspace(), properties);
  updateProperty("OutputWorkspace", workspaces.iVsQ(), properties);
  updateProperty("OutputWorkspaceBinned", workspaces.iVsQBinned(), properties);
  updateProperty("OutputWorkspaceWavelength", workspaces.iVsLambda(),
                 properties);
  updateProperty("FirstTransmissionRun", workspaces.transmissionRuns().first,
                 properties);
  updateProperty("SecondTransmissionRun", workspaces.transmissionRuns().second,
                 properties);
}

void updateRangeInQProperties(AlgorithmRuntimeProps &properties,
                              RangeInQ const &rangeInQ) {
  updateProperty("MomentumTransferMin", rangeInQ.min(), properties);
  updateProperty("MomentumTransferMax", rangeInQ.max(), properties);
  updateProperty("MomentumTransferStep", rangeInQ.step(), properties);
}

void updateRowProperties(AlgorithmRuntimeProps &properties, Row const &row) {
  updateWorkspacesNameProperties(properties, row.reducedWorkspaceNames());
  updateRangeInQProperties(properties, row.qRange());
  updateProperty("ThetaIn", row.theta(), properties);
  updateProperty("ScaleFactor", row.scaleFactor(), properties);
}

void updateExperimentProperties(AlgorithmRuntimeProps &properties,
                                Experiment const &experiment) {
  updateProperty("AnalysisMode",
                 analysisModeToString(experiment.analysisMode()), properties);
  updateProperty("ReductionType",
                 reductionTypeToString(experiment.reductionType()), properties);
  updateProperty("SummationType",
                 summationTypeToString(experiment.summationType()), properties);
  updateProperty("IncludePartialBins",
                 boolToString(experiment.includePartialBins()), properties);
  updateProperty("Debug", boolToString(experiment.debug()), properties);
}

void addAlgorithmForRow(Row &row, Batch const &model,
                        BatchAlgorithmRunner &batchAlgoRunner) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryReductionOneAuto");

  auto properties = AlgorithmRuntimeProps();
  // updateEventProperties(properties, model.experiment());
  updateExperimentProperties(properties, model.experiment());
  // updateInstrumentProperties(properties, model.experiment());
  // updateSaveProperties(properties, model.experiment());
  updateRowProperties(properties, row);

  batchAlgoRunner.addAlgorithm(alg, properties, &row);
}

void addAlgorithmsForGroup(Group &group, Batch const &model,
                           BatchAlgorithmRunner &batchAlgoRunner,
                           bool reprocessFailed, bool processAll) {
  auto &rows = group.rows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(reprocessFailed) &&
        (processAll || model.isSelected(row.get())))
      addAlgorithmForRow(row.get(), model, batchAlgoRunner);
  }
}
} // namespace unnamed

BatchJobRunner::BatchJobRunner(Batch batch,
                               BatchAlgorithmRunner &batchAlgoRunner)
    : m_batch(std::move(batch)), m_isProcessing(false), m_isAutoreducing(false),
      m_reprocessFailed(false), m_batchAlgoRunner(batchAlgoRunner) {
  m_batchAlgoRunner.stopOnFailure(false);
}

bool BatchJobRunner::isProcessing() const { return m_isProcessing; }

bool BatchJobRunner::isAutoreducing() const { return m_isAutoreducing; }

void BatchJobRunner::resumeReduction() {
  m_isProcessing = true;
  m_reprocessFailed = false;
  m_processAll = !m_batch.hasSelection();
  setUpBatchAlgorithmRunner();
}

void BatchJobRunner::reductionPaused() { m_isProcessing = false; }

void BatchJobRunner::resumeAutoreduction() {
  m_isAutoreducing = true;
  m_isProcessing = true;
  m_reprocessFailed = true;
  m_processAll = true;
  setUpBatchAlgorithmRunner();
}

void BatchJobRunner::autoreductionPaused() { m_isAutoreducing = false; }

void BatchJobRunner::setReprocessFailedItems(bool reprocessFailed) {
  m_reprocessFailed = reprocessFailed;
}

void BatchJobRunner::setUpBatchAlgorithmRunner() {
  m_batchAlgoRunner.clearQueue();

  auto &groups = m_batch.runsTable().reductionJobs().groups();
  for (auto &group : groups)
    addAlgorithmsForGroup(group, m_batch, m_batchAlgoRunner, m_reprocessFailed,
                          m_processAll);
}
} // namespace CustomInterfaces
} // namespace MantidQt
