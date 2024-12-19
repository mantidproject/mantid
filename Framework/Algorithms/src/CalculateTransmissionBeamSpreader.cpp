// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CalculateTransmissionBeamSpreader.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateTransmissionBeamSpreader)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using std::size_t;

void CalculateTransmissionBeamSpreader::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<CommonBinsValidator>();
  wsValidator->add<HistogramValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("SampleSpreaderRunWorkspace", "", Direction::Input, wsValidator),
      "The workspace containing the sample beam-spreader run");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("DirectSpreaderRunWorkspace", "", Direction::Input, wsValidator),
      "The workspace containing the direct beam-spreader run");
  declareProperty(std::make_unique<WorkspaceProperty<>>("SampleScatterRunWorkspace", "", Direction::Input, wsValidator),
                  "The workspace containing the sample scattering run");
  declareProperty(std::make_unique<WorkspaceProperty<>>("DirectScatterRunWorkspace", "", Direction::Input, wsValidator),
                  "The workspace containing the direct beam scattering run");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The fitted transmission correction");

  auto zeroOrMore = std::make_shared<BoundedValidator<int>>();
  zeroOrMore->setLower(0);
  // The defaults here are the correct detector numbers for LOQ
  declareProperty("IncidentBeamMonitor", 2, zeroOrMore, "The UDET of the incident beam monitor");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty("SpreaderTransmissionValue", 1.0, mustBePositive, "Transmission coefficient of the beam spreader");
  declareProperty("SpreaderTransmissionError", 0.0, mustBePositive,
                  "Uncertainty on the transmission coefficient of the beam spreader");

  declareProperty("MinWavelength", 2.2, mustBePositive, "The minimum wavelength for the fit");
  declareProperty("MaxWavelength", 10.0, mustBePositive, "The maximum wavelength for the fit");

  std::vector<std::string> options(2);
  options[0] = "Linear";
  options[1] = "Log";
  declareProperty("FitMethod", "Log", std::make_shared<StringListValidator>(options),
                  "Whether to fit directly to the transmission curve (Linear) "
                  "or to the log of it (Log)");

  declareProperty("OutputUnfittedData", false);
}

void CalculateTransmissionBeamSpreader::exec() {
  MatrixWorkspace_sptr sample_spreaderWS = getProperty("SampleSpreaderRunWorkspace");
  MatrixWorkspace_sptr direct_spreaderWS = getProperty("DirectSpreaderRunWorkspace");
  MatrixWorkspace_sptr sample_scatterWS = getProperty("SampleScatterRunWorkspace");
  MatrixWorkspace_sptr direct_scatterWS = getProperty("DirectScatterRunWorkspace");

  // Check that the two input workspaces are from the same instrument
  if (sample_spreaderWS->getInstrument()->getName() != direct_spreaderWS->getInstrument()->getName() ||
      sample_spreaderWS->getInstrument()->getName() != sample_scatterWS->getInstrument()->getName() ||
      sample_spreaderWS->getInstrument()->getName() != direct_scatterWS->getInstrument()->getName()) {
    g_log.error("The input workspaces do not come from the same instrument");
    throw std::invalid_argument("The input workspaces do not come from the same instrument");
  }
  // Check that the two inputs have matching binning
  if (!WorkspaceHelpers::matchingBins(sample_spreaderWS, direct_spreaderWS) ||
      !WorkspaceHelpers::matchingBins(sample_spreaderWS, sample_scatterWS) ||
      !WorkspaceHelpers::matchingBins(sample_spreaderWS, direct_scatterWS)) {
    g_log.error("Input workspaces do not have matching binning");
    throw std::invalid_argument("Input workspaces do not have matching binning");
  }

  // Extract the required spectra into separate workspaces
  // The static_cast should not be necessary but it is required to avoid a
  // "internal compiler error: segmentation fault" when compiling with gcc
  // and std=c++1z
  std::vector<detid_t> udets{static_cast<detid_t>(getProperty("IncidentBeamMonitor"))};

  // Convert UDETs to workspace indices
  // Get monitors (assume that the detector mapping is the same for all data
  // sets)
  std::vector<size_t> indices = sample_scatterWS->getIndicesFromDetectorIDs(udets);
  if (indices.size() != 1) {
    g_log.error() << "Could not find the incident monitor spectra\n";
    throw std::invalid_argument("Could not find the incident monitor spectra\n");
  }

  MatrixWorkspace_sptr sample_scatter_mon = this->extractSpectrum(sample_scatterWS, indices[0]);
  MatrixWorkspace_sptr direct_scatter_mon = this->extractSpectrum(direct_scatterWS, indices[0]);
  MatrixWorkspace_sptr sample_spreader_mon = this->extractSpectrum(sample_spreaderWS, indices[0]);
  MatrixWorkspace_sptr direct_spreader_mon = this->extractSpectrum(direct_spreaderWS, indices[0]);

  // Sum the whole detector for each of the four data sets
  MatrixWorkspace_sptr sample_scatter_sum;
  MatrixWorkspace_sptr direct_scatter_sum;
  MatrixWorkspace_sptr sample_spreader_sum;
  MatrixWorkspace_sptr direct_spreader_sum;

  // Note: Replaced PARALLEL_SECTION with this OMP for loop, due to occasional
  // unexplained segfault.
  std::vector<MatrixWorkspace_sptr> in_ws{sample_scatterWS, direct_scatterWS, sample_spreaderWS, direct_spreaderWS};

  std::vector<MatrixWorkspace_sptr> out_ws(4);

  PARALLEL_FOR_IF(true)
  for (int i = 0; i < 4; i++) {
    out_ws[i] = this->sumSpectra(in_ws[i]);
  }
  sample_scatter_sum = out_ws[0];
  direct_scatter_sum = out_ws[1];
  sample_spreader_sum = out_ws[2];
  direct_spreader_sum = out_ws[3];

  // Beam spreader transmission
  MatrixWorkspace_sptr spreader_trans = create<WorkspaceSingleValue>(1, Points(1));
  spreader_trans->setYUnit("");
  spreader_trans->setDistribution(true);
  spreader_trans->mutableX(0)[0] = 0.0;
  spreader_trans->mutableY(0)[0] = getProperty("SpreaderTransmissionValue");
  spreader_trans->mutableE(0)[0] = getProperty("SpreaderTransmissionError");

  // The main calculation
  MatrixWorkspace_sptr numerator =
      sample_spreader_sum / sample_spreader_mon - spreader_trans * sample_scatter_sum / sample_scatter_mon;

  MatrixWorkspace_sptr denominator =
      direct_spreader_sum / direct_spreader_mon - spreader_trans * direct_scatter_sum / direct_scatter_mon;

  MatrixWorkspace_sptr transmission = numerator / denominator;

  // Output this data if requested
  const bool outputRaw = getProperty("OutputUnfittedData");
  if (outputRaw) {
    std::string outputWSName = getPropertyValue("OutputWorkspace");
    outputWSName += "_unfitted";
    declareProperty(std::make_unique<WorkspaceProperty<>>("UnfittedData", outputWSName, Direction::Output));
    setProperty("UnfittedData", transmission);
  }

  // Check that there are more than a single bin in the transmission
  // workspace. Skip the fit it there isn't.
  if (transmission->y(0).size() == 1) {
    setProperty("OutputWorkspace", transmission);
  } else {
    MatrixWorkspace_sptr fit;
    const std::string fitMethod = getProperty("FitMethod");
    logFit = (fitMethod == "Log");
    if (logFit) {
      g_log.debug("Fitting to the logarithm of the transmission");
      // Take a copy of this workspace for the fitting
      MatrixWorkspace_sptr logTransmission = this->extractSpectrum(transmission, 0);

      // Take the log of each datapoint for fitting. Preserve errors
      // percentage-wise.
      auto &Y = logTransmission->mutableY(0);
      auto &E = logTransmission->mutableE(0);
      Progress progress(this, 0.4, 0.6, Y.size());
      for (size_t i = 0; i < Y.size(); ++i) {
        E[i] = std::abs(E[i] / Y[i]);
        Y[i] = std::log10(Y[i]);
        progress.report("Calculate Transmission");
      }

      // Now fit this to a straight line
      fit = this->fitToData(logTransmission);
    } // logFit true
    else {
      g_log.debug("Fitting directly to the data (i.e. linearly)");
      fit = this->fitToData(transmission);
    }

    setProperty("OutputWorkspace", fit);
  }
}

/** Sum all detector pixels except monitors and masked detectors
 *  @param WS ::    The workspace containing the spectrum to sum
 *  @return A Workspace2D containing the sum
 */
API::MatrixWorkspace_sptr CalculateTransmissionBeamSpreader::sumSpectra(const API::MatrixWorkspace_sptr &WS) {
  Algorithm_sptr childAlg = createChildAlgorithm("SumSpectra");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<bool>("IncludeMonitors", false);
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/** Extracts a single spectrum from a Workspace2D into a new workspaces. Uses
 * CropWorkspace to do this.
 *  @param WS ::    The workspace containing the spectrum to extract
 *  @param index :: The workspace index of the spectrum to extract
 *  @return A Workspace2D containing the extracted spectrum
 */
API::MatrixWorkspace_sptr CalculateTransmissionBeamSpreader::extractSpectrum(const API::MatrixWorkspace_sptr &WS,
                                                                             const size_t index) {
  // Check that given spectra are monitors
  if (!WS->spectrumInfo().isMonitor(index)) {
    g_log.information("The Incident Beam Monitor UDET provided is not marked as a monitor");
  }

  Algorithm_sptr childAlg = createChildAlgorithm("ExtractSingleSpectrum", 0.0, 0.4);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex", static_cast<int>(index));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/** Uses 'Linear' as a ChildAlgorithm to fit the log of the exponential curve
 * expected for the transmission.
 *  @param WS :: The single-spectrum workspace to fit
 *  @return A workspace containing the fit
 */
API::MatrixWorkspace_sptr CalculateTransmissionBeamSpreader::fitToData(const API::MatrixWorkspace_sptr &WS) {
  g_log.information("Fitting the experimental transmission curve");
  Algorithm_sptr childAlg = createChildAlgorithm("Linear", 0.6, 1.0);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  const double lambdaMin = getProperty("MinWavelength");
  const double lambdaMax = getProperty("MaxWavelength");
  childAlg->setProperty<double>("StartX", lambdaMin);
  childAlg->setProperty<double>("EndX", lambdaMax);
  childAlg->executeAsChildAlg();

  std::string fitStatus = childAlg->getProperty("FitStatus");
  if (fitStatus != "success") {
    g_log.error("Unable to successfully fit the data: " + fitStatus);
    throw std::runtime_error("Unable to successfully fit the data");
  }

  // Only get to here if successful
  MatrixWorkspace_sptr result = childAlg->getProperty("OutputWorkspace");

  if (logFit) {
    // Need to transform back to 'unlogged'
    double b = childAlg->getProperty("FitIntercept");
    double m = childAlg->getProperty("FitSlope");
    b = std::pow(10, b);
    m = std::pow(10, m);

    auto X = result->points(0);
    auto &Y = result->mutableY(0);
    auto &E = result->mutableE(0);
    for (size_t i = 0; i < Y.size(); ++i) {
      Y[i] = b * (std::pow(m, X[i]));
      E[i] = std::abs(E[i] * Y[i]);
    }
  }

  return result;
}

} // namespace Mantid::Algorithms
