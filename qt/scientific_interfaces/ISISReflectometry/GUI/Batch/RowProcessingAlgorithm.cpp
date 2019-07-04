// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "RowProcessingAlgorithm.h"
#include "../../Reduction/Batch.h"
#include "../../Reduction/Row.h"
#include "AlgorithmProperties.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
using AlgorithmRuntimeProps = std::map<std::string, std::string>;

namespace { // unnamed namespace
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
      "PolarizationAnalysis",
      polarizationCorrectionTypeToString(corrections.correctionType()),
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
      floodCorrectionTypeToString(corrections.correctionType()), properties);

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
    // No valid slicing so there is nothing to do
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
    // If we don't have an interval, there's nothing to do. Also, we don't
    // currently support multiple intervals, so skip that as well.
    if (slicing.sliceAtValues().size() < 1 ||
        slicing.sliceAtValues().size() > 1)
      return;

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

boost::optional<double> getDouble(IAlgorithm_sptr algorithm,
                                  std::string const &property) {
  double result = algorithm->getProperty(property);
  return result;
}

void updateRowFromOutputProperties(IAlgorithm_sptr algorithm, Item &item) {
  auto &row = dynamic_cast<Row &>(item);

  auto const iVsLam = AlgorithmProperties::getOutputWorkspace(
      algorithm, "OutputWorkspaceWavelength");
  auto const iVsQ =
      AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspace");
  auto const iVsQBin = AlgorithmProperties::getOutputWorkspace(
      algorithm, "OutputWorkspaceBinned");
  row.setOutputNames(std::vector<std::string>{iVsLam, iVsQ, iVsQBin});

  auto qRange = RangeInQ(getDouble(algorithm, "MomentumTransferMin"),
                         getDouble(algorithm, "MomentumTransferStep"),
                         getDouble(algorithm, "MomentumTransferMax"));
  row.setOutputQRange(qRange);
}
} // unnamed namespace

/** Create a configured algorithm for processing a row. The algorithm
 * properties are set from the reduction configuration model and the
 * cell values in the given row.
 * @param model : the reduction configuration model
 * @param row : the row from the runs table
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(Batch const &model,
                                                    Row &row) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryISISLoadAndProcess");
  alg->setRethrows(true);

  // Set the algorithm properties from the model
  auto properties = createAlgorithmRuntimeProps(model, row);

  // Return the configured algorithm
  auto jobAlgorithm = boost::make_shared<BatchJobAlgorithm>(
      alg, properties, updateRowFromOutputProperties, &row);
  return jobAlgorithm;
}

AlgorithmRuntimeProps createAlgorithmRuntimeProps(Batch const &model,
                                                  Row const &row) {
  // Create properties for the model
  auto properties = createAlgorithmRuntimeProps(model);
  // Update properties specific to this row - the per-angle options based on
  // the known angle, and the values in the table cells in the row
  updatePerThetaDefaultProperties(properties,
                                  model.defaultsForTheta(row.theta()));
  updateRowProperties(properties, row);
  return properties;
}

AlgorithmRuntimeProps createAlgorithmRuntimeProps(Batch const &model) {
  auto properties = AlgorithmRuntimeProps();
  // Update properties from settings in the event, experiment and instrument
  // tabs
  updateEventProperties(properties, model.slicing());
  updateExperimentProperties(properties, model.experiment());
  updateInstrumentProperties(properties, model.instrument());
  // Update properties from the wildcard row in the per-theta defaults table
  updatePerThetaDefaultProperties(properties, model.wildcardDefaults());
  return properties;
}
} // namespace CustomInterfaces
} // namespace MantidQt
