// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobRunner.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::ConfiguredAlgorithm;
using API::BatchAlgorithmRunnerSubscriber;
using Mantid::API::IAlgorithm_sptr;
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

template <typename VALUE_TYPE>
void updateProperty(std::string const &property,
                    std::vector<VALUE_TYPE> const &values,
                    AlgorithmRuntimeProps &properties) {
  if (values.size() < 1)
    return;

  auto value =
      Mantid::Kernel::Strings::simpleJoin(values.cbegin(), values.cend(), ", ");
  updateProperty(property, value, properties);
}

void updateProperty(std::string const &property, bool value,
                    AlgorithmRuntimeProps &properties) {
  updateProperty(property, boolToString(value), properties);
}

void updateProperty(std::string const &property, int value,
                    AlgorithmRuntimeProps &properties) {
  updateProperty(property, std::to_string(value), properties);
}

void updateProperty(std::string const &property, size_t value,
                    AlgorithmRuntimeProps &properties) {
  updateProperty(property, std::to_string(value), properties);
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

void updateMomentumTransferProperties(AlgorithmRuntimeProps &properties,
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
  updateMomentumTransferProperties(properties, row.qRange());
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
  updateMomentumTransferProperties(properties, perThetaDefaults->qRange());
  updateProperty("ScaleFactor", perThetaDefaults->scaleFactor(), properties);
  updateProperty("ProcessingInstructions",
                 perThetaDefaults->processingInstructions(), properties);
}

void updateWavelengthRangeProperties(
    AlgorithmRuntimeProps &properties,
    boost::optional<RangeInLambda> const &rangeInLambda) {
  if (!rangeInLambda)
    return;

  updateProperty("WavelengthMin", rangeInLambda->min(), properties);
  updateProperty("WavelengthMax", rangeInLambda->max(), properties);
}

void updateMonitorCorrectionProperties(AlgorithmRuntimeProps &properties,
                                       MonitorCorrections const &monitor) {
  updateProperty("I0MonitorIndex", monitor.monitorIndex(), properties);
  updateProperty("NormalizeByIntegratedMonitors", monitor.integrate(),
                 properties);
  if (monitor.integralRange() && monitor.integralRange()->minSet())
    updateProperty("MonitorIntegrationWavelengthMin",
                   monitor.integralRange()->min(), properties);
  if (monitor.integralRange() && monitor.integralRange()->maxSet())
    updateProperty("MonitorIntegrationWavelengthMax",
                   monitor.integralRange()->max(), properties);
  if (monitor.backgroundRange() && monitor.backgroundRange()->minSet())
    updateProperty("MonitorBackgroundWavelengthMin",
                   monitor.backgroundRange()->min(), properties);
  if (monitor.backgroundRange() && monitor.backgroundRange()->maxSet())
    updateProperty("MonitorBackgroundWavelengthMax",
                   monitor.backgroundRange()->max(), properties);
}

void updateDetectorCorrectionProperties(AlgorithmRuntimeProps &properties,
                                        DetectorCorrections const &detector) {
  updateProperty("CorrectDetectors", detector.correctPositions(), properties);
  if (detector.correctPositions())
    updateProperty("DetectorCorrectionType",
                   detectorCorrectionTypeToString(detector.correctionType()),
                   properties);
}

void updateInstrumentProperties(AlgorithmRuntimeProps &properties,
                                Instrument const &instrument) {
  updateWavelengthRangeProperties(properties, instrument.wavelengthRange());
  updateMonitorCorrectionProperties(properties,
                                    instrument.monitorCorrections());
  updateDetectorCorrectionProperties(properties,
                                     instrument.detectorCorrections());
}

class UpdateEventPropertiesVisitor : public boost::static_visitor<> {
public:
  explicit UpdateEventPropertiesVisitor(AlgorithmRuntimeProps &properties)
      : m_properties(properties) {}
  void operator()(boost::blank const &) const {
    // No slicing specified so there is nothing to do
  }
  void operator()(InvalidSlicing const &) const {
    throw std::runtime_error("Program error: Invalid slicing");
  }
  void operator()(UniformSlicingByTime const &slicing) const {
    enableSlicing();
    updateProperty("TimeInterval", slicing.sliceLengthInSeconds(),
                   m_properties);
  }
  void operator()(UniformSlicingByNumberOfSlices const &slicing) const {
    enableSlicing();
    updateProperty("NumberOfSlices", slicing.numberOfSlices(), m_properties);
  }
  void operator()(CustomSlicingByList const &slicing) const {
    enableSlicing();
    updateProperty("TimeInterval", slicing.sliceTimes(), m_properties);
  }
  void operator()(SlicingByEventLog const &slicing) const {
    if (slicing.sliceAtValues().size() < 1)
      return;
    if (slicing.sliceAtValues().size() > 1)
      throw std::runtime_error("Custom log value intervals are not "
                               "implemented; please specify a single "
                               "interval width");
    enableSlicing();
    updateProperty("LogName", slicing.blockName(), m_properties);
    updateProperty("LogValueInterval", slicing.sliceAtValues()[0],
                   m_properties);
  }

private:
  AlgorithmRuntimeProps &m_properties;

  void enableSlicing() const {
    updateProperty("SliceWorkspace", true, m_properties);
  }
};

void updateEventProperties(AlgorithmRuntimeProps &properties,
                           Slicing const &slicing) {
  boost::apply_visitor(UpdateEventPropertiesVisitor(properties), slicing);
}

ConfiguredAlgorithm getAlgorithmForRow(Row &row, Batch const &model) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryISISLoadAndProcess");

  auto properties = AlgorithmRuntimeProps();
  updateEventProperties(properties, model.slicing());
  updateExperimentProperties(properties, model.experiment());
  updatePerThetaDefaultProperties(properties,
                                  model.defaultsForTheta(row.theta()));
  updateInstrumentProperties(properties, model.instrument());
  // updateSaveProperties(properties, model.experiment());
  updateRowProperties(properties, row);
  return ConfiguredAlgorithm(alg, properties, &row);
}

std::deque<ConfiguredAlgorithm> getAlgorithmsForGroup(Group &group,
                                                      Batch const &model,
                                                      bool reprocessFailed,
                                                      bool processAll) {
  auto algorithms = std::deque<ConfiguredAlgorithm>();
  auto &rows = group.mutableRows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(reprocessFailed) &&
        (processAll || model.isSelected(row.get())))
      algorithms.emplace_back(getAlgorithmForRow(row.get(), model));
  }
  return algorithms;
}
} // namespace

BatchJobRunner::BatchJobRunner(Batch batch)
    : m_batch(std::move(batch)), m_isProcessing(false), m_isAutoreducing(false),
      m_reprocessFailed(false) {}

bool BatchJobRunner::isProcessing() const { return m_isProcessing; }

bool BatchJobRunner::isAutoreducing() const { return m_isAutoreducing; }

void BatchJobRunner::resumeReduction() {
  m_isProcessing = true;
  // If the user has manually selected failed rows, reprocess them; otherwise
  // skip them
  m_reprocessFailed = m_batch.hasSelection();
  // If there are no selected rows, process everything
  m_processAll = !m_batch.hasSelection();
}

void BatchJobRunner::reductionPaused() { m_isProcessing = false; }

void BatchJobRunner::resumeAutoreduction() {
  m_isAutoreducing = true;
  m_isProcessing = true;
  m_reprocessFailed = true;
  m_processAll = true;
}

void BatchJobRunner::autoreductionPaused() { m_isAutoreducing = false; }

void BatchJobRunner::setReprocessFailedItems(bool reprocessFailed) {
  m_reprocessFailed = reprocessFailed;
}

std::deque<ConfiguredAlgorithm> BatchJobRunner::getAlgorithms() {
  auto algorithms = std::deque<ConfiguredAlgorithm>();
  auto &groups =
      m_batch.mutableRunsTable().mutableReductionJobs().mutableGroups();
  for (auto &group : groups) {
    auto groupAlgorithms =
        getAlgorithmsForGroup(group, m_batch, m_reprocessFailed, m_processAll);
    algorithms.insert(algorithms.end(),
                      std::make_move_iterator(groupAlgorithms.begin()),
                      std::make_move_iterator(groupAlgorithms.end()));
  }
  return algorithms;
}

void BatchJobRunner::algorithmFinished(IAlgorithm_sptr algorithm) {
  UNUSED_ARG(algorithm);
}

void BatchJobRunner::algorithmError(std::string const &message,
                                    IAlgorithm_sptr algorithm) {
  UNUSED_ARG(message);
  UNUSED_ARG(algorithm);
}

std::vector<std::string> BatchJobRunner::algorithmOutputWorkspacesToSave(
    IAlgorithm_sptr algorithm,
    const BatchAlgorithmRunnerSubscriber *const item) const {

  if (algorithm->name() == "ReflectometryISISLoadAndProcess") {
    auto const *row = dynamic_cast<Row const *>(item);
    if (!row)
      throw std::runtime_error("Internal error: invalid row type");
    return getWorkspacesToSave(*row);
  } else if (algorithm->name() == "Stitch1DMany") {
    auto const *group = dynamic_cast<Group const *>(item);
    if (!group)
      throw std::runtime_error("Internal error: invalid group type");
    return getWorkspacesToSave(*group);
  }
  return std::vector<std::string>();
}

std::vector<std::string>
BatchJobRunner::getWorkspacesToSave(Group const &group) const {
  return std::vector<std::string>{group.postprocessedWorkspaceName()};
}

std::vector<std::string>
BatchJobRunner::getWorkspacesToSave(Row const &row) const {
  // Get the output workspaces for the given row. Note that we only save
  // workspaces for the row if the group does not have postprocessing, because
  // in that case users just want to see the postprocessed output instead.
  auto workspaces = std::vector<std::string>();
  auto const group = m_batch.runsTable().reductionJobs().getParentGroup(row);
  if (group.requiresPostprocessing())
    return workspaces;

  // We currently only save the binned workspace in Q
  workspaces.push_back(row.reducedWorkspaceNames().iVsQBinned());
  return workspaces;
}

void BatchJobRunner::notifyWorkspaceDeleted(std::string const &wsName) {
  // Reset the state for the relevant row if the workspace was one of our
  // outputs
  auto item = m_batch.getItemWithOutputWorkspaceOrNone(wsName);
  if (item)
    item->resetState();
}

void BatchJobRunner::notifyWorkspaceRenamed(std::string const &oldName,
                                            std::string const &newName) {
  // Update the workspace name in the model, if it is one of our outputs
  auto item = m_batch.getItemWithOutputWorkspaceOrNone(oldName);
  if (item)
    item->renameOutputWorkspace(oldName, newName);
}

void BatchJobRunner::notifyAllWorkspacesDeleted() {
  // All output workspaces will be deleted so reset all rows and groups
  m_batch.resetState();
}
} // namespace CustomInterfaces
} // namespace MantidQt
