// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobRunner.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Strings.h"
#include <typeinfo>

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;
using AlgorithmRuntimeProps = std::map<std::string, std::string>;

namespace AlgorithmProperties {
// These convenience functions convert properties of various types into
// strings to set the relevant property in an AlgorithmRuntimeProps
std::string boolToString(bool value) { return value ? "1" : "0"; }

void update(std::string const &property, std::string const &value,
            AlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties[property] = value;
}

void update(std::string const &property,
            boost::optional<std::string> const &value,
            AlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

template <typename VALUE_TYPE>
void update(std::string const &property, std::vector<VALUE_TYPE> const &values,
            AlgorithmRuntimeProps &properties) {
  if (values.size() < 1)
    return;

  auto value =
      Mantid::Kernel::Strings::simpleJoin(values.cbegin(), values.cend(), ", ");
  update(property, value, properties);
}

void update(std::string const &property, bool value,
            AlgorithmRuntimeProps &properties) {
  update(property, boolToString(value), properties);
}

void update(std::string const &property, int value,
            AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, size_t value,
            AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, double value,
            AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, boost::optional<double> const &value,
            AlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void updateFromMap(AlgorithmRuntimeProps &properties,
                   std::map<std::string, std::string> const &parameterMap) {
  for (auto kvp : parameterMap) {
    update(kvp.first, kvp.second, properties);
  }
}
} // namespace AlgorithmProperties

namespace RowProperties {
// These functions update properties in an AlgorithmRuntimeProps for specific
// properties for the row reduction algorithm
void updateInputWorkspacesProperties(
    AlgorithmRuntimeProps &properties,
    std::vector<std::string> const &inputRunNumbers) {
  AlgorithmProperties::update("InputRunList", inputRunNumbers, properties);
}

void updateTransmissionWorkspaceProperties(
    AlgorithmRuntimeProps &properties,
    TransmissionRunPair const &transmissionRuns) {
  AlgorithmProperties::update("FirstTransmissionRunList",
                              transmissionRuns.firstRunList(), properties);
  AlgorithmProperties::update("SecondTransmissionRunList",
                              transmissionRuns.secondRunList(), properties);
}

void updateMomentumTransferProperties(AlgorithmRuntimeProps &properties,
                                      RangeInQ const &rangeInQ) {
  AlgorithmProperties::update("MomentumTransferMin", rangeInQ.min(),
                              properties);
  AlgorithmProperties::update("MomentumTransferMax", rangeInQ.max(),
                              properties);
  AlgorithmProperties::update("MomentumTransferStep", rangeInQ.step(),
                              properties);
}

void updateRowProperties(AlgorithmRuntimeProps &properties, Row const &row) {
  updateInputWorkspacesProperties(
      properties, row.reducedWorkspaceNames().inputRunNumbers());
  updateTransmissionWorkspaceProperties(
      properties, row.reducedWorkspaceNames().transmissionRuns());
  updateMomentumTransferProperties(properties, row.qRange());
  AlgorithmProperties::update("ThetaIn", row.theta(), properties);
  AlgorithmProperties::update("ScaleFactor", row.scaleFactor(), properties);
  AlgorithmProperties::updateFromMap(properties, row.reductionOptions());
}

void updateTransmissionRangeProperties(
    AlgorithmRuntimeProps &properties,
    boost::optional<RangeInLambda> const &range) {
  if (!range)
    return;

  if (range->minSet())
    AlgorithmProperties::update("StartOverlap", range->min(), properties);

  if (range->maxSet())
    AlgorithmProperties::update("EndOverlap", range->max(), properties);
}

void updatePolarizationCorrectionProperties(
    AlgorithmRuntimeProps &properties,
    PolarizationCorrections const &corrections) {
  if (corrections.correctionType() == PolarizationCorrectionType::None)
    return;

  AlgorithmProperties::update(
      "PolarisationCorrections",
      PolarizationCorrectionTypeToString(corrections.correctionType()),
      properties);

  if (corrections.correctionType() == PolarizationCorrectionType::PA ||
      corrections.correctionType() == PolarizationCorrectionType::PNR) {
    AlgorithmProperties::update("CRho", corrections.cRho(), properties);
    AlgorithmProperties::update("CAlpha", corrections.cRho(), properties);
    AlgorithmProperties::update("CAp", corrections.cRho(), properties);
    AlgorithmProperties::update("CPp", corrections.cRho(), properties);
  }
}

void updateFloodCorrectionProperties(AlgorithmRuntimeProps &properties,
                                     FloodCorrections const &corrections) {
  AlgorithmProperties::update(
      "FloodCorrection",
      FloodCorrectionTypeToString(corrections.correctionType()), properties);

  if (corrections.correctionType() == FloodCorrectionType::Workspace)
    AlgorithmProperties::update("FloodWorkspace", corrections.workspace(),
                                properties);
}

void updateExperimentProperties(AlgorithmRuntimeProps &properties,
                                Experiment const &experiment) {
  AlgorithmProperties::update("AnalysisMode",
                              analysisModeToString(experiment.analysisMode()),
                              properties);
  AlgorithmProperties::update("Debug", experiment.debug(), properties);
  AlgorithmProperties::update("SummationType",
                              summationTypeToString(experiment.summationType()),
                              properties);
  AlgorithmProperties::update("ReductionType",
                              reductionTypeToString(experiment.reductionType()),
                              properties);
  AlgorithmProperties::update("IncludePartialBins",
                              experiment.includePartialBins(), properties);
  updateTransmissionRangeProperties(properties,
                                    experiment.transmissionRunRange());
  updatePolarizationCorrectionProperties(properties,
                                         experiment.polarizationCorrections());
  updateFloodCorrectionProperties(properties, experiment.floodCorrections());
  AlgorithmProperties::updateFromMap(properties, experiment.stitchParameters());
}

void updatePerThetaDefaultProperties(AlgorithmRuntimeProps &properties,
                                     PerThetaDefaults const *perThetaDefaults) {
  if (!perThetaDefaults)
    return;

  updateTransmissionWorkspaceProperties(
      properties, perThetaDefaults->transmissionWorkspaceNames());
  updateMomentumTransferProperties(properties, perThetaDefaults->qRange());
  AlgorithmProperties::update("ScaleFactor", perThetaDefaults->scaleFactor(),
                              properties);
  AlgorithmProperties::update("ProcessingInstructions",
                              perThetaDefaults->processingInstructions(),
                              properties);
}

void updateWavelengthRangeProperties(
    AlgorithmRuntimeProps &properties,
    boost::optional<RangeInLambda> const &rangeInLambda) {
  if (!rangeInLambda)
    return;

  AlgorithmProperties::update("WavelengthMin", rangeInLambda->min(),
                              properties);
  AlgorithmProperties::update("WavelengthMax", rangeInLambda->max(),
                              properties);
}

void updateMonitorCorrectionProperties(AlgorithmRuntimeProps &properties,
                                       MonitorCorrections const &monitor) {
  AlgorithmProperties::update("I0MonitorIndex", monitor.monitorIndex(),
                              properties);
  AlgorithmProperties::update("NormalizeByIntegratedMonitors",
                              monitor.integrate(), properties);
  if (monitor.integralRange() && monitor.integralRange()->minSet())
    AlgorithmProperties::update("MonitorIntegrationWavelengthMin",
                                monitor.integralRange()->min(), properties);
  if (monitor.integralRange() && monitor.integralRange()->maxSet())
    AlgorithmProperties::update("MonitorIntegrationWavelengthMax",
                                monitor.integralRange()->max(), properties);
  if (monitor.backgroundRange() && monitor.backgroundRange()->minSet())
    AlgorithmProperties::update("MonitorBackgroundWavelengthMin",
                                monitor.backgroundRange()->min(), properties);
  if (monitor.backgroundRange() && monitor.backgroundRange()->maxSet())
    AlgorithmProperties::update("MonitorBackgroundWavelengthMax",
                                monitor.backgroundRange()->max(), properties);
}

void updateDetectorCorrectionProperties(AlgorithmRuntimeProps &properties,
                                        DetectorCorrections const &detector) {
  AlgorithmProperties::update("CorrectDetectors", detector.correctPositions(),
                              properties);
  if (detector.correctPositions())
    AlgorithmProperties::update(
        "DetectorCorrectionType",
        detectorCorrectionTypeToString(detector.correctionType()), properties);
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
    AlgorithmProperties::update("TimeInterval", slicing.sliceLengthInSeconds(),
                                m_properties);
  }
  void operator()(UniformSlicingByNumberOfSlices const &slicing) const {
    enableSlicing();
    AlgorithmProperties::update("NumberOfSlices", slicing.numberOfSlices(),
                                m_properties);
  }
  void operator()(CustomSlicingByList const &slicing) const {
    enableSlicing();
    AlgorithmProperties::update("TimeInterval", slicing.sliceTimes(),
                                m_properties);
  }
  void operator()(SlicingByEventLog const &slicing) const {
    if (slicing.sliceAtValues().size() < 1)
      return;
    if (slicing.sliceAtValues().size() > 1)
      throw std::runtime_error("Custom log value intervals are not "
                               "implemented; please specify a single "
                               "interval width");
    enableSlicing();
    AlgorithmProperties::update("LogName", slicing.blockName(), m_properties);
    AlgorithmProperties::update("LogValueInterval", slicing.sliceAtValues()[0],
                                m_properties);
  }

private:
  AlgorithmRuntimeProps &m_properties;

  void enableSlicing() const {
    AlgorithmProperties::update("SliceWorkspace", true, m_properties);
  }
};

void updateEventProperties(AlgorithmRuntimeProps &properties,
                           Slicing const &slicing) {
  boost::apply_visitor(UpdateEventPropertiesVisitor(properties), slicing);
}
} // namespace RowProperties

BatchJobAlgorithm::BatchJobAlgorithm(
    Mantid::API::IAlgorithm_sptr algorithm,
    MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps properties,
    // cppcheck-suppress passedByValue
    std::vector<std::string> outputWorkspaceProperties, Item *item)
    : ConfiguredAlgorithm(algorithm, properties), m_item(item),
      m_outputWorkspaceProperties(outputWorkspaceProperties) {}

Item *BatchJobAlgorithm::item() { return m_item; }

std::vector<std::string> BatchJobAlgorithm::outputWorkspaceNames() const {
  auto workspaceNames = std::vector<std::string>();
  for (auto &property : m_outputWorkspaceProperties) {
    workspaceNames.emplace_back(m_algorithm->getPropertyValue(property));
  }
  return workspaceNames;
}

std::map<std::string, Workspace_sptr>
BatchJobAlgorithm::outputWorkspaceNameToWorkspace() const {
  auto propertyToName = std::map<std::string, Workspace_sptr>();
  for (auto &property : m_outputWorkspaceProperties) {
    auto workspaceName = m_algorithm->getPropertyValue(property);
    if (!workspaceName.empty())
      propertyToName[workspaceName] = m_algorithm->getProperty(property);
  }
  return propertyToName;
}

BatchJobRunner::BatchJobRunner(Batch batch)
    : m_batch(std::move(batch)), m_isProcessing(false), m_isAutoreducing(false),
      m_reprocessFailed(false), m_processAll(false) {}

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

/** Get algorithms and related properties for processing rows and groups
 * in the table
 */
std::deque<IConfiguredAlgorithm_sptr> BatchJobRunner::getAlgorithms() {
  auto algorithms = std::deque<IConfiguredAlgorithm_sptr>();
  auto &groups =
      m_batch.mutableRunsTable().mutableReductionJobs().mutableGroups();
  for (auto &group : groups) {
    addAlgorithmsForRowsInGroup(group, algorithms);
  }
  return algorithms;
}

/** Add the algorithms and related properties for processing all the rows
 * in a group
 * @param group : the group to get the row algorithms for
 * @param algorithms : the list of configured algorithms to add this group's
 * rows to
 */
void BatchJobRunner::addAlgorithmsForRowsInGroup(
    Group &group, std::deque<IConfiguredAlgorithm_sptr> &algorithms) {
  auto &rows = group.mutableRows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(m_reprocessFailed) &&
        (m_processAll || m_batch.isSelected(row.get()))) {
      addAlgorithmForRow(row.get(), algorithms);
    }
  }
}

/** Add the algorithm and related properties for processing a row
 * @param row : the row to get the configured algorithm for
 * @param algorithms : the list of configured algorithms to add this row to
 */
void BatchJobRunner::addAlgorithmForRow(
    Row &row, std::deque<IConfiguredAlgorithm_sptr> &algorithms) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryISISLoadAndProcess");
  alg->setChild(true);

  // Set up input properties
  auto properties = AlgorithmRuntimeProps();
  RowProperties::updateEventProperties(properties, m_batch.slicing());
  RowProperties::updateExperimentProperties(properties, m_batch.experiment());
  RowProperties::updatePerThetaDefaultProperties(
      properties, m_batch.defaultsForTheta(row.theta()));
  RowProperties::updateInstrumentProperties(properties, m_batch.instrument());
  RowProperties::updateRowProperties(properties, row);

  // Store expected output property names. Must be in the correct order for
  // Row::algorithmComplete
  std::vector<std::string> outputWorkspaceProperties = {
      "OutputWorkspaceWavelength", "OutputWorkspace", "OutputWorkspaceBinned"};

  // Add the configured algorithm to the list
  auto jobAlgorithm = boost::make_shared<BatchJobAlgorithm>(
      alg, properties, outputWorkspaceProperties, &row);
  algorithms.emplace_back(std::move(jobAlgorithm));
}

void BatchJobRunner::algorithmStarted(IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->algorithmStarted();
}

void BatchJobRunner::algorithmComplete(IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->algorithmComplete(jobAlgorithm->outputWorkspaceNames());

  for (auto &kvp : jobAlgorithm->outputWorkspaceNameToWorkspace()) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(kvp.first,
                                                              kvp.second);
  }
}

void BatchJobRunner::algorithmError(IConfiguredAlgorithm_sptr algorithm,
                                    std::string const &message) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->algorithmError(message);
}

std::vector<std::string> BatchJobRunner::algorithmOutputWorkspacesToSave(
    IConfiguredAlgorithm_sptr algorithm) const {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();

  if (item->isGroup())
    return getWorkspacesToSave(dynamic_cast<Group &>(*item));
  else
    return getWorkspacesToSave(dynamic_cast<Row &>(*item));

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
