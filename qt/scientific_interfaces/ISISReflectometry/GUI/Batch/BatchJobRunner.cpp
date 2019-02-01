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

using API::BatchAlgorithmRunner;
using Mantid::API::IAlgorithm;
using AlgorithmRuntimeProps = std::map<std::string, std::string>;

namespace { // unnamed
std::string boolToString(bool value) { return value ? "1" : "0"; }

void updateProperty(std::string const &property, std::string const &value,
                    AlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties[property] = value;
}

void updateProperty(std::string const &property,
                    boost::optional<std::string> const &value,
                    AlgorithmRuntimeProps &properties) {
  if (value)
    updateProperty(property, value.get(), properties);
}

void updateProperty(std::string const &property,
                    std::vector<std::string> const &values,
                    AlgorithmRuntimeProps &properties) {
  if (values.size() < 1)
    return;

  auto value = boost::algorithm::join(values, ", ");
  updateProperty(property, value, properties);
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

void updatePropertiesFromMap(
    AlgorithmRuntimeProps &properties,
    std::map<std::string, std::string> const &parameterMap) {
  for (auto kvp : parameterMap) {
    updateProperty(kvp.first, kvp.second, properties);
  }
}

void updateInputWorkspacesProperties(
    AlgorithmRuntimeProps &properties,
    std::vector<std::string> const &inputRunNumbers) {
  updateProperty("InputRunList", inputRunNumbers, properties);
}

void updateTransmissionWorkspaceProperties(
    AlgorithmRuntimeProps &properties,
    TransmissionRunPair const &transmissionRuns) {
  updateProperty("FirstTransmissionRunList", transmissionRuns.firstRunList(),
                 properties);
  updateProperty("SecondTransmissionRunList", transmissionRuns.secondRunList(),
                 properties);
}

void updateRangeInQProperties(AlgorithmRuntimeProps &properties,
                              RangeInQ const &rangeInQ) {
  updateProperty("MomentumTransferMin", rangeInQ.min(), properties);
  updateProperty("MomentumTransferMax", rangeInQ.max(), properties);
  updateProperty("MomentumTransferStep", rangeInQ.step(), properties);
}

void updateRowProperties(AlgorithmRuntimeProps &properties, Row const &row) {
  updateInputWorkspacesProperties(
      properties, row.reducedWorkspaceNames().inputRunNumbers());
  updateTransmissionWorkspaceProperties(
      properties, row.reducedWorkspaceNames().transmissionRuns());
  updateRangeInQProperties(properties, row.qRange());
  updateProperty("ThetaIn", row.theta(), properties);
  updateProperty("ScaleFactor", row.scaleFactor(), properties);
  updatePropertiesFromMap(properties, row.reductionOptions());
}

void updateTransmissionRangeProperties(
    AlgorithmRuntimeProps &properties,
    boost::optional<RangeInLambda> const &range) {
  if (!range)
    return;

  if (range->minSet())
    updateProperty("StartOverlap", range->min(), properties);

  if (range->maxSet())
    updateProperty("EndOverlap", range->max(), properties);
}

void updatePolarizationCorrectionProperties(
    AlgorithmRuntimeProps &properties,
    PolarizationCorrections const &corrections) {
  if (corrections.correctionType() == PolarizationCorrectionType::None)
    return;

  updateProperty(
      "PolarisationCorrections",
      PolarizationCorrectionTypeToString(corrections.correctionType()),
      properties);

  if (corrections.correctionType() == PolarizationCorrectionType::PA ||
      corrections.correctionType() == PolarizationCorrectionType::PNR) {
    updateProperty("CRho", corrections.cRho(), properties);
    updateProperty("CAlpha", corrections.cRho(), properties);
    updateProperty("CAp", corrections.cRho(), properties);
    updateProperty("CPp", corrections.cRho(), properties);
  }
}

void updateFloodCorrectionProperties(AlgorithmRuntimeProps &properties,
                                     FloodCorrections const &corrections) {
  updateProperty("FloodCorrection",
                 FloodCorrectionTypeToString(corrections.correctionType()),
                 properties);

  if (corrections.correctionType() == FloodCorrectionType::Workspace)
    updateProperty("FloodWorkspace", corrections.workspace(), properties);
}

void updateExperimentProperties(AlgorithmRuntimeProps &properties,
                                Experiment const &experiment) {
  updateProperty("AnalysisMode",
                 analysisModeToString(experiment.analysisMode()), properties);
  updateProperty("Debug", boolToString(experiment.debug()), properties);
  updateProperty("SummationType",
                 summationTypeToString(experiment.summationType()), properties);
  updateProperty("ReductionType",
                 reductionTypeToString(experiment.reductionType()), properties);
  updateProperty("IncludePartialBins",
                 boolToString(experiment.includePartialBins()), properties);
  updateTransmissionRangeProperties(properties,
                                    experiment.transmissionRunRange());
  updatePolarizationCorrectionProperties(properties,
                                         experiment.polarizationCorrections());
  updateFloodCorrectionProperties(properties, experiment.floodCorrections());
  updatePropertiesFromMap(properties, experiment.stitchParameters());
}

void updatePerThetaDefaultProperties(AlgorithmRuntimeProps &properties,
                                     PerThetaDefaults const *perThetaDefaults) {
  if (!perThetaDefaults)
    return;

  updateTransmissionWorkspaceProperties(
      properties, perThetaDefaults->transmissionWorkspaceNames());
  updateRangeInQProperties(properties, perThetaDefaults->qRange());
  updateProperty("ScaleFactor", perThetaDefaults->scaleFactor(), properties);
  updateProperty("ProcessingInstructions",
                 perThetaDefaults->processingInstructions(), properties);
}

void addAlgorithmForRow(Row &row, Batch const &model,
                        BatchAlgorithmRunner &batchAlgoRunner) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryISISLoadAndProcess");

  auto properties = AlgorithmRuntimeProps();
  // updateEventProperties(properties, model.experiment());
  updateExperimentProperties(properties, model.experiment());
  updatePerThetaDefaultProperties(properties,
                                  model.defaultsForTheta(row.theta()));
  // updateInstrumentProperties(properties, model.experiment());
  // updateSaveProperties(properties, model.experiment());
  updateRowProperties(properties, row);

  batchAlgoRunner.addAlgorithm(alg, properties, &row);
}

void addAlgorithmsForGroup(Group &group, Batch const &model,
                           BatchAlgorithmRunner &batchAlgoRunner,
                           bool reprocessFailed, bool processAll) {
  auto &rows = group.mutableRows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(reprocessFailed) &&
        (processAll || model.isSelected(row.get())))
      addAlgorithmForRow(row.get(), model, batchAlgoRunner);
  }
}
} // namespace

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
  // If the user has manually selected failed rows, reprocess them; otherwise
  // skip them
  m_reprocessFailed = m_batch.hasSelection();
  // If there are no selected rows, process everything
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

  auto &groups =
      m_batch.mutableRunsTable().mutableReductionJobs().mutableGroups();
  for (auto &group : groups)
    addAlgorithmsForGroup(group, m_batch, m_batchAlgoRunner, m_reprocessFailed,
                          m_processAll);
}
} // namespace CustomInterfaces
} // namespace MantidQt
