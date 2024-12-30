// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RowProcessingAlgorithm.h"
#include "../../GUI/Preview/ROIType.h"
#include "../../Reduction/Batch.h"
#include "../../Reduction/PreviewRow.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using Mantid::API::AlgorithmRuntimeProps;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using MantidQt::API::IConfiguredAlgorithm_sptr;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry RowProcessingAlgorithm");
} // namespace

namespace { // unnamed namespace
using namespace Mantid::API;

// These functions update properties in an AlgorithmRuntimeProps for specific
// properties for the row reduction algorithm
void updateInputWorkspacesProperties(AlgorithmRuntimeProps &properties,
                                     std::vector<std::string> const &inputRunNumbers) {
  AlgorithmProperties::update("InputRunList", inputRunNumbers, properties);
}

void updateTransmissionWorkspaceProperties(AlgorithmRuntimeProps &properties,
                                           TransmissionRunPair const &transmissionRuns) {
  // Transmission runs come as a pair: if the first is set, use both; else use neither
  if (transmissionRuns.firstRunList().empty()) {
    return;
  }
  properties.setPropertyValue("FirstTransmissionRunList", transmissionRuns.firstRunList());
  properties.setPropertyValue("SecondTransmissionRunList", transmissionRuns.secondRunList());
}

void updateMomentumTransferProperties(AlgorithmRuntimeProps &properties, RangeInQ const &rangeInQ) {
  AlgorithmProperties::update("MomentumTransferMin", rangeInQ.min(), properties);
  AlgorithmProperties::update("MomentumTransferMax", rangeInQ.max(), properties);
  AlgorithmProperties::update("MomentumTransferStep", rangeInQ.step(), properties);
}

void updateProcessingInstructionsProperties(AlgorithmRuntimeProps &properties, PreviewRow const &previewRow) {
  AlgorithmProperties::update("ProcessingInstructions", previewRow.getProcessingInstructions(ROIType::Signal),
                              properties);
  AlgorithmProperties::update("BackgroundProcessingInstructions",
                              previewRow.getProcessingInstructions(ROIType::Background), properties);
  AlgorithmProperties::update("TransmissionProcessingInstructions",
                              previewRow.getProcessingInstructions(ROIType::Transmission), properties);
}

void updateRowProperties(AlgorithmRuntimeProps &properties, Row const &row) {
  updateInputWorkspacesProperties(properties, row.reducedWorkspaceNames().inputRunNumbers());
  updateTransmissionWorkspaceProperties(properties, row.reducedWorkspaceNames().transmissionRuns());
  updateMomentumTransferProperties(properties, row.qRange());
  AlgorithmProperties::update("ThetaIn", row.theta(), properties);
  AlgorithmProperties::update("ScaleFactor", row.scaleFactor(), properties);
  AlgorithmProperties::updateFromMap(properties, row.reductionOptions());
}

void updateTransmissionStitchProperties(AlgorithmRuntimeProps &properties, TransmissionStitchOptions const &options) {
  auto range = options.overlapRange();
  if (range) {
    if (range->minSet())
      AlgorithmProperties::update("StartOverlap", range->min(), properties);

    if (range->maxSet())
      AlgorithmProperties::update("EndOverlap", range->max(), properties);
  }

  if (!options.rebinParameters().empty()) {
    AlgorithmProperties::update("Params", options.rebinParameters(), properties);
  }

  AlgorithmProperties::update("ScaleRHSWorkspace", options.scaleRHS(), properties);
}

void updateBackgroundSubtractionProperties(AlgorithmRuntimeProps &properties,
                                           BackgroundSubtraction const &subtraction) {
  AlgorithmProperties::update("SubtractBackground", subtraction.subtractBackground(), properties);
  AlgorithmProperties::update("BackgroundCalculationMethod",
                              backgroundSubtractionTypeToString(subtraction.subtractionType()), properties);
  AlgorithmProperties::update("DegreeOfPolynomial", subtraction.degreeOfPolynomial(), properties);
  AlgorithmProperties::update("CostFunction", costFunctionTypeToString(subtraction.costFunction()), properties);
}

void updatePolarizationCorrectionProperties(AlgorithmRuntimeProps &properties,
                                            PolarizationCorrections const &corrections) {
  // None or set to workspace or filepath with no workspace or path given.
  if (corrections.correctionType() == PolarizationCorrectionType::None ||
      (corrections.correctionType() == PolarizationCorrectionType::Workspace && corrections.workspace()->empty()))
    return;

  // Use the parameter file.
  AlgorithmProperties::update("PolarizationAnalysis", true, properties);

  // Use the supplied workspace.
  if (corrections.correctionType() == PolarizationCorrectionType::Workspace) {
    AlgorithmProperties::update("PolarizationEfficiencies", corrections.workspace(), properties);
  }
}

void updateFloodCorrectionProperties(AlgorithmRuntimeProps &properties, FloodCorrections const &corrections) {
  AlgorithmProperties::update("FloodCorrection", floodCorrectionTypeToString(corrections.correctionType()), properties);

  if (corrections.correctionType() == FloodCorrectionType::Workspace)
    AlgorithmProperties::update("FloodWorkspace", corrections.workspace(), properties);
}

void updateExperimentProperties(AlgorithmRuntimeProps &properties, Experiment const &experiment) {
  AlgorithmProperties::update("AnalysisMode", analysisModeToString(experiment.analysisMode()), properties);
  AlgorithmProperties::update("Debug", experiment.debug(), properties);
  SummationType summationType = experiment.summationType();
  AlgorithmProperties::update("SummationType", summationTypeToString(summationType), properties);
  // The ReductionType value is only relevant when the SummationType is SumInQ
  ReductionType reductionType =
      (summationType == SummationType::SumInQ) ? experiment.reductionType() : ReductionType::Normal;
  AlgorithmProperties::update("ReductionType", reductionTypeToString(reductionType), properties);
  AlgorithmProperties::update("IncludePartialBins", experiment.includePartialBins(), properties);
  updateTransmissionStitchProperties(properties, experiment.transmissionStitchOptions());
  updateBackgroundSubtractionProperties(properties, experiment.backgroundSubtraction());
  updatePolarizationCorrectionProperties(properties, experiment.polarizationCorrections());
  updateFloodCorrectionProperties(properties, experiment.floodCorrections());
}

void updateLookupRowProperties(AlgorithmRuntimeProps &properties, LookupRow const &lookupRow) {
  updateTransmissionWorkspaceProperties(properties, lookupRow.transmissionWorkspaceNames());
  AlgorithmProperties::update("TransmissionProcessingInstructions", lookupRow.transmissionProcessingInstructions(),
                              properties);
  updateMomentumTransferProperties(properties, lookupRow.qRange());
  AlgorithmProperties::update("ScaleFactor", lookupRow.scaleFactor(), properties);
  AlgorithmProperties::update("ProcessingInstructions", lookupRow.processingInstructions(), properties);
  AlgorithmProperties::update("BackgroundProcessingInstructions", lookupRow.backgroundProcessingInstructions(),
                              properties);
  AlgorithmProperties::update("ROIDetectorIDs", lookupRow.roiDetectorIDs(), properties);
}

void updateWavelengthRangeProperties(AlgorithmRuntimeProps &properties,
                                     boost::optional<RangeInLambda> const &rangeInLambda) {
  if (!rangeInLambda)
    return;

  AlgorithmProperties::update("WavelengthMin", rangeInLambda->min(), properties);
  AlgorithmProperties::update("WavelengthMax", rangeInLambda->max(), properties);
}

void updateMonitorCorrectionProperties(AlgorithmRuntimeProps &properties, MonitorCorrections const &monitor) {
  AlgorithmProperties::update("I0MonitorIndex", monitor.monitorIndex(), properties);
  AlgorithmProperties::update("NormalizeByIntegratedMonitors", monitor.integrate(), properties);
  if (monitor.integralRange() && monitor.integralRange()->minSet())
    AlgorithmProperties::update("MonitorIntegrationWavelengthMin", monitor.integralRange()->min(), properties);
  if (monitor.integralRange() && monitor.integralRange()->maxSet())
    AlgorithmProperties::update("MonitorIntegrationWavelengthMax", monitor.integralRange()->max(), properties);
  if (monitor.backgroundRange() && monitor.backgroundRange()->minSet())
    AlgorithmProperties::update("MonitorBackgroundWavelengthMin", monitor.backgroundRange()->min(), properties);
  if (monitor.backgroundRange() && monitor.backgroundRange()->maxSet())
    AlgorithmProperties::update("MonitorBackgroundWavelengthMax", monitor.backgroundRange()->max(), properties);
}

void updateDetectorCorrectionProperties(AlgorithmRuntimeProps &properties, DetectorCorrections const &detector) {
  AlgorithmProperties::update("CorrectDetectors", detector.correctPositions(), properties);
  if (detector.correctPositions())
    AlgorithmProperties::update("DetectorCorrectionType", detectorCorrectionTypeToString(detector.correctionType()),
                                properties);
}

void updatePreviewInstrumentProperties(AlgorithmRuntimeProps &properties, Instrument const &instrument) {
  updateWavelengthRangeProperties(properties, instrument.wavelengthRange());
  updateMonitorCorrectionProperties(properties, instrument.monitorCorrections());
  updateDetectorCorrectionProperties(properties, instrument.detectorCorrections());
}

void updateInstrumentProperties(AlgorithmRuntimeProps &properties, Instrument const &instrument) {
  updatePreviewInstrumentProperties(properties, instrument);
  AlgorithmProperties::update("CalibrationFile", instrument.calibrationFilePath(), properties);
}

class UpdateEventPropertiesVisitor : public boost::static_visitor<> {
public:
  explicit UpdateEventPropertiesVisitor(AlgorithmRuntimeProps &properties) : m_properties(properties) {}
  void operator()(boost::blank const &) const {
    // No slicing specified so there is nothing to do
  }
  void operator()(InvalidSlicing const &) const {
    // No valid slicing so there is nothing to do
  }
  void operator()(UniformSlicingByTime const &slicing) const {
    enableSlicing();
    AlgorithmProperties::update("TimeInterval", slicing.sliceLengthInSeconds(), m_properties);
  }
  void operator()(UniformSlicingByNumberOfSlices const &slicing) const {
    enableSlicing();
    AlgorithmProperties::update("NumberOfSlices", slicing.numberOfSlices(), m_properties);
  }
  void operator()(CustomSlicingByList const &slicing) const {
    enableSlicing();
    AlgorithmProperties::update("TimeInterval", slicing.sliceTimes(), m_properties);
  }
  void operator()(SlicingByEventLog const &slicing) const {
    // If we don't have an interval, there's nothing to do. Also, we don't
    // currently support multiple intervals, so skip that as well.
    if (slicing.sliceAtValues().size() < 1 || slicing.sliceAtValues().size() > 1)
      return;

    enableSlicing();
    AlgorithmProperties::update("LogName", slicing.blockName(), m_properties);
    AlgorithmProperties::update("LogValueInterval", slicing.sliceAtValues()[0], m_properties);
  }

private:
  AlgorithmRuntimeProps &m_properties;

  void enableSlicing() const { AlgorithmProperties::update("SliceWorkspace", true, m_properties); }
};

void updateEventProperties(AlgorithmRuntimeProps &properties, Slicing const &slicing) {
  boost::apply_visitor(UpdateEventPropertiesVisitor(properties), slicing);
}

std::optional<double> getDouble(const IAlgorithm_sptr &algorithm, std::string const &property) {
  double result = algorithm->getProperty(property);
  return result;
}

void updateRowFromOutputProperties(const IAlgorithm_sptr &algorithm, Item &item) {
  auto &row = dynamic_cast<Row &>(item);

  auto const iVsLam = AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspaceWavelength");
  auto const iVsQ = AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspace");
  auto const iVsQBin = AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspaceBinned");
  row.setOutputNames(std::vector<std::string>{iVsLam, iVsQ, iVsQBin});

  auto qRange = RangeInQ(getDouble(algorithm, "MomentumTransferMin"), getDouble(algorithm, "MomentumTransferStep"),
                         getDouble(algorithm, "MomentumTransferMax"));
  row.setOutputQRange(qRange);
}

// Get the lookup row from the model. Because using a wildcard row or algorithm defaults
// can be confusing this function also logs warnings about what is happening.
boost::optional<LookupRow> findLookupRow(Row const &row, IBatch const &model) {
  auto lookupRow = model.findLookupRow(row);
  if (!lookupRow) {
    g_log.warning(
        "No matching experiment settings found for " + boost::algorithm::join(row.runNumbers(), ", ") +
        ". Using algorithm default settings instead. You may wish to check that all lookup criteria on the Experiment "
        "Settings table are correct.");
  } else if (lookupRow->isWildcard()) {
    g_log.warning(
        "No matching experiment settings found for " + boost::algorithm::join(row.runNumbers(), ", ") +
        ". Using wildcard row settings instead. You may wish to check that all lookup criteria on the Experiment "
        "Settings table are correct.");
  }
  return lookupRow;
}

// Get the wildcard lookup row from the model. Because using a wildcard row or algorithm defaults
// can be confusing this function also logs warnings about what is happening.
boost::optional<LookupRow> findWildcardLookupRow(IBatch const &model) {
  auto lookupRow = model.findWildcardLookupRow();
  if (lookupRow) {
    g_log.warning("Using experiment settings from the wildcard row.");
  } else {
    g_log.warning("No experiment settings found; using algorithm defaults. You may wish to specify a wildcard"
                  " row in the Experiment Settings table to override them.");
  }
  return lookupRow;
}

// Set properties from the batch model
void updatePropertiesFromBatchModel(AlgorithmRuntimeProps &properties, IBatch const &model) {
  // Update properties from settings in the event, experiment and instrument tabs
  updateEventProperties(properties, model.slicing());
  updateExperimentProperties(properties, model.experiment());
  updateInstrumentProperties(properties, model.instrument());
}

void updatePreviewPropertiesFromBatchModel(AlgorithmRuntimeProps &properties, IBatch const &model) {
  // Update properties for running a preview reduction from settings in the event, experiment and instrument tabs
  updateEventProperties(properties, model.slicing());
  updateExperimentProperties(properties, model.experiment());
  updatePreviewInstrumentProperties(properties, model.instrument());
}
} // unnamed namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry::Reduction {
/** Create a configured algorithm for processing a row. The algorithm
 * properties are set from the reduction configuration model and the
 * cell values in the given row.
 * @param model : the reduction configuration model
 * @param row : the row from the preview tab
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model, PreviewRow &row,
                                                    Mantid::API::IAlgorithm_sptr alg) {
  // Create the algorithm
  if (!alg) {
    alg = Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISLoadAndProcess");
  }
  alg->setRethrows(true);
  alg->setAlwaysStoreInADS(false);
  alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();

  // Set the algorithm properties from the model
  auto properties = createAlgorithmRuntimeProps(model, row);

  // Return the configured algorithm
  auto jobAlgorithm =
      std::make_shared<BatchJobAlgorithm>(std::move(alg), std::move(properties), updateRowOnAlgorithmComplete, &row);
  return jobAlgorithm;
}

/** This function gets the canonical set of properties for performing the reduction, either using defaults for all runs
 * or for a specific run if that run's Row is passed. It starts with the most generic set of defaults, overrides them
 * from the lookup table if a match is found there, and then finally overrides them with the specific run's settings if
 * the user has specified them on the Runs table.
 *
 * @param model : the Batch model containing all of the default settings and the lookup table
 * @param row : optional run details from the Runs table
 * @returns : a custom PropertyManager class with all of the algorithm properties set
 */
std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> createAlgorithmRuntimeProps(IBatch const &model,
                                                                                 PreviewRow const &previewRow) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  updatePreviewPropertiesFromBatchModel(*properties, model);
  // Look up properties for this run on the lookup table
  auto lookupRow = model.findLookupRow(previewRow);
  if (lookupRow) {
    updateLookupRowProperties(*properties, *lookupRow);
  }
  // Update properties from the preview tab
  properties->setProperty("InputRunList", previewRow.runNumbers());
  properties->setProperty("ThetaIn", previewRow.theta());
  if (previewRow.getSelectedBanks().has_value()) {
    properties->setProperty("ROIDetectorIDs", previewRow.getSelectedBanks().get());
  }
  updateProcessingInstructionsProperties(*properties, previewRow);

  properties->setProperty("HideInputWorkspaces", true);
  return properties;
}

void updateRowOnAlgorithmComplete(const IAlgorithm_sptr &algorithm, Item &item) {
  auto &row = dynamic_cast<PreviewRow &>(item);
  MatrixWorkspace_sptr outputWs = algorithm->getProperty("OutputWorkspaceBinned");
  row.setReducedWs(outputWs);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::Reduction

namespace MantidQt::CustomInterfaces::ISISReflectometry::RowProcessing {
/** Create a configured algorithm for processing a row. The algorithm
 * properties are set from the reduction configuration model and the
 * cell values in the given row.
 * @param model : the reduction configuration model
 * @param row : the row from the runs table
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model, Row &row) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISLoadAndProcess");
  alg->setRethrows(true);

  // Set the algorithm properties from the model
  auto properties = createAlgorithmRuntimeProps(model, row);

  // Return the configured algorithm
  auto jobAlgorithm =
      std::make_shared<BatchJobAlgorithm>(std::move(alg), std::move(properties), updateRowFromOutputProperties, &row);
  return jobAlgorithm;
}

/** This function gets the canonical set of properties for performing the reduction, either using defaults for all runs
 * or for a specific run if that run's Row is passed. It starts with the most generic set of defaults, overrides them
 * from the lookup table if a match is found there, and then finally overrides them with the specific run's settings if
 * the user has specified them on the Runs table.
 *
 * @param model : the Batch model containing all of the default settings and the lookup table
 * @param row : optional run details from the Runs table
 * @returns : a custom PropertyManager class with all of the algorithm properties set
 */
std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> createAlgorithmRuntimeProps(IBatch const &model,
                                                                                 boost::optional<Row const &> row) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  // Update properties from settings in the event, experiment and instrument tabs
  updatePropertiesFromBatchModel(*properties, model);
  // Look up properties for this run on the lookup table (or use wildcard defaults if no run is given)
  auto lookupRow = row ? findLookupRow(*row, model) : findWildcardLookupRow(model);
  if (lookupRow) {
    updateLookupRowProperties(*properties, *lookupRow);
  }
  // Update properties the user has specifically set for this run
  if (row) {
    updateRowProperties(*properties, *row);
  }
  return properties;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::RowProcessing
