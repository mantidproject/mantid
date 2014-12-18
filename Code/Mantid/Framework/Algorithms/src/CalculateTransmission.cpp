//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

#include <cmath>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateTransmission)

using namespace Kernel;
using namespace API;

CalculateTransmission::CalculateTransmission()
    : API::Algorithm(), m_done(0.0) {}

CalculateTransmission::~CalculateTransmission() {}

void CalculateTransmission::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<CommonBinsValidator>();
  wsValidator->add<HistogramValidator>();

  declareProperty(new WorkspaceProperty<>("SampleRunWorkspace", "",
                                          Direction::Input, wsValidator),
                  "The workspace containing the sample transmission run. Must "
                  "have common binning and be in units of wavelength.");
  declareProperty(new WorkspaceProperty<>("DirectRunWorkspace", "",
                                          Direction::Input, wsValidator),
                  "The workspace containing the direct beam (no sample) "
                  "transmission run. The units and binning must match those of "
                  "the SampleRunWorkspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the workspace in which to store the fitted transmission "
      "fractions.");

  auto zeroOrMore = boost::make_shared<BoundedValidator<int>>();
  zeroOrMore->setLower(0);
  // The defaults here are the correct detector numbers for LOQ
  declareProperty("IncidentBeamMonitor", EMPTY_INT(),
                  "The UDET of the incident beam monitor");
  declareProperty("TransmissionMonitor", 3, zeroOrMore,
                  "The UDET of the transmission monitor");

  declareProperty(new ArrayProperty<double>("RebinParams"),
                  "A comma separated list of first bin boundary, width, last "
                  "bin boundary. Optionally\n"
                  "this can be followed by a comma and more widths and last "
                  "boundary pairs.\n"
                  "Negative width values indicate logarithmic binning.");

  std::vector<std::string> options(3);
  options[0] = "Linear";
  options[1] = "Log";
  options[2] = "Polynomial";

  declareProperty("FitMethod", "Log",
                  boost::make_shared<StringListValidator>(options),
                  "Whether to fit directly to the transmission curve using "
                  "Linear, Log or Polynomial.");
  auto twoOrMore = boost::make_shared<BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("PolynomialOrder", 2, twoOrMore, "Order of the polynomial to "
                                                   "fit. It is considered only "
                                                   "for FitMethod=Polynomial");

  declareProperty("OutputUnfittedData", false,
                  "If True, will output an additional workspace called "
                  "[OutputWorkspace]_unfitted containing the unfitted "
                  "transmission correction.");
}

void CalculateTransmission::exec() {
  MatrixWorkspace_sptr sampleWS = getProperty("SampleRunWorkspace");
  MatrixWorkspace_sptr directWS = getProperty("DirectRunWorkspace");

  // Check whether we need to normalise by the beam monitor
  int beamMonitorID = getProperty("IncidentBeamMonitor");
  bool normaliseToMonitor = true;
  if (isEmpty(beamMonitorID))
    normaliseToMonitor = false;
  else if (beamMonitorID < 0) {
    g_log.error("The beam monitor UDET should be greater or equal to zero");
    throw std::invalid_argument(
        "The beam monitor UDET should be greater or equal to zero");
  }

  // Check that the two input workspaces are from the same instrument
  if (sampleWS->getInstrument()->getName() !=
      directWS->getInstrument()->getName()) {
    g_log.error("The input workspaces do not come from the same instrument");
    throw std::invalid_argument(
        "The input workspaces do not come from the same instrument");
  }
  // Check that the two inputs have matching binning
  if (!WorkspaceHelpers::matchingBins(sampleWS, directWS)) {
    g_log.error("Input workspaces do not have matching binning");
    throw std::invalid_argument(
        "Input workspaces do not have matching binning");
  }

  // Extract the required spectra into separate workspaces
  std::vector<detid_t> udets;
  std::vector<size_t> indices;
  // For LOQ at least, the incident beam monitor's UDET is 2 and the
  // transmission monitor is 3
  udets.push_back(getProperty("TransmissionMonitor"));
  if (normaliseToMonitor)
    udets.push_back(getProperty("IncidentBeamMonitor"));
  // Convert UDETs to workspace indices
  sampleWS->getIndicesFromDetectorIDs(udets, indices);
  if ((indices.size() < 2 && normaliseToMonitor) ||
      (indices.size() < 1 && !normaliseToMonitor)) {
    if (indices.size() == 1) {
      g_log.error() << "Incident and transmitted spectra must be set to "
                       "different spectra that exist in the workspaces. Only "
                       "found one valid index " << indices.front() << std::endl;
    } else {
      g_log.debug() << "sampleWS->getIndicesFromDetectorIDs() returned empty\n";
    }
    throw std::invalid_argument(
        "Could not find the incident and transmission monitor spectra\n");
  }
  // Check that given spectra are monitors
  if (normaliseToMonitor &&
      !sampleWS->getDetector(indices.back())->isMonitor()) {
    g_log.information(
        "The Incident Beam Monitor UDET provided is not marked as a monitor");
  }
  if (!sampleWS->getDetector(indices.front())->isMonitor()) {
    g_log.information(
        "The Transmission Monitor UDET provided is not marked as a monitor");
  }
  MatrixWorkspace_sptr M2_sample;
  if (normaliseToMonitor)
    M2_sample = this->extractSpectrum(sampleWS, indices[1]);
  MatrixWorkspace_sptr M3_sample = this->extractSpectrum(sampleWS, indices[0]);
  sampleWS->getIndicesFromDetectorIDs(udets, indices);
  // Check that given spectra are monitors
  if (!directWS->getDetector(indices.back())->isMonitor()) {
    g_log.information(
        "The Incident Beam Monitor UDET provided is not marked as a monitor");
  }
  if (!directWS->getDetector(indices.front())->isMonitor()) {
    g_log.information(
        "The Transmission Monitor UDET provided is not marked as a monitor");
  }
  MatrixWorkspace_sptr M2_direct;
  if (normaliseToMonitor)
    M2_direct = this->extractSpectrum(directWS, indices[1]);
  MatrixWorkspace_sptr M3_direct = this->extractSpectrum(directWS, indices[0]);

  double start = m_done;
  Progress progress(this, start, m_done += 0.2, 2);
  progress.report("CalculateTransmission: Dividing transmission by incident");
  // The main calculation
  MatrixWorkspace_sptr transmission = M3_sample / M3_direct;
  if (normaliseToMonitor)
    transmission = transmission * (M2_direct / M2_sample);

  // This workspace is now a distribution
  progress.report("CalculateTransmission: Dividing transmission by incident");

  // Output this data if requested
  const bool outputRaw = getProperty("OutputUnfittedData");
  if (outputRaw) {
    IAlgorithm_sptr childAlg = createChildAlgorithm("ReplaceSpecialValues");
    childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", transmission);
    childAlg->setProperty<double>("NaNValue", 0.0);
    childAlg->setProperty<double>("NaNError", 0.0);
    childAlg->setProperty<double>("InfinityValue", 0.0);
    childAlg->setProperty<double>("InfinityError", 0.0);
    childAlg->executeAsChildAlg();
    transmission = childAlg->getProperty("OutputWorkspace");
    std::string outputWSName = getPropertyValue("OutputWorkspace");
    outputWSName += "_unfitted";
    declareProperty(new WorkspaceProperty<>("UnfittedData", outputWSName,
                                            Direction::Output));
    setProperty("UnfittedData", transmission);
  }

  // Check that there are more than a single bin in the transmission
  // workspace. Skip the fit it there isn't.
  if (transmission->dataY(0).size() > 1) {
    transmission =
        fit(transmission, getProperty("RebinParams"), getProperty("FitMethod"));
  }
  setProperty("OutputWorkspace", transmission);
}

/** Extracts a single spectrum from a Workspace2D into a new workspaces. Uses
 * CropWorkspace to do this.
 *  @param WS ::    The workspace containing the spectrum to extract
 *  @param index :: The workspace index of the spectrum to extract
 *  @return A Workspace2D containing the extracted spectrum
 *  @throw runtime_error if the ExtractSingleSpectrum algorithm fails during
 * execution
 */
API::MatrixWorkspace_sptr
CalculateTransmission::extractSpectrum(API::MatrixWorkspace_sptr WS,
                                       const int64_t index) {
  double start = m_done;
  IAlgorithm_sptr childAlg =
      createChildAlgorithm("ExtractSingleSpectrum", start, m_done += 0.1);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex", static_cast<int>(index));
  childAlg->executeAsChildAlg();

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}
/** Calculate a workspace that contains the result of the fit to the
* transmission fraction that was calculated
*  @param raw [in] the workspace with the unfitted transmission ratio data
*  @param rebinParams [in] the parameters for rebinning
*  @param fitMethod [in] string can be Log, Linear, Poly2, Poly3, Poly4, Poly5,
* Poly6
*  @return a workspace that contains the evaluation of the fit
*  @throw runtime_error if the Linear or ExtractSpectrum algorithm fails during
* execution
*/
API::MatrixWorkspace_sptr
CalculateTransmission::fit(API::MatrixWorkspace_sptr raw,
                           std::vector<double> rebinParams,
                           const std::string fitMethod) {
  MatrixWorkspace_sptr output = this->extractSpectrum(raw, 0);

  Progress progress(this, m_done, 1.0, 4);
  progress.report("CalculateTransmission: Performing fit");

  // these are calculated by the call to fit below
  double grad(0.0), offset(0.0);
  std::vector<double> coeficients;
  const bool logFit = (fitMethod == "Log");
  if (logFit) {
    g_log.debug("Fitting to the logarithm of the transmission");

    MantidVec &Y = output->dataY(0);
    MantidVec &E = output->dataE(0);
    double start = m_done;
    Progress prog2(this, start, m_done += 0.1, Y.size());
    for (size_t i = 0; i < Y.size(); ++i) {
      // Take the log of each datapoint for fitting. Recalculate errors
      // remembering that d(log(a))/da  = 1/a
      E[i] = std::abs(E[i] / Y[i]);
      Y[i] = std::log10(Y[i]);
      progress.report("Fitting to the logarithm of the transmission");
    }

    // Now fit this to a straight line
    output = fitData(output, grad, offset);
  }                                 // logFit true
  else if (fitMethod == "Linear") { // Linear fit
    g_log.debug("Fitting directly to the data (i.e. linearly)");
    output = fitData(output, grad, offset);
  } else { // fitMethod Polynomial
    int order = getProperty("PolynomialOrder");
    std::stringstream info;
    info << "Fitting the transmission to polynomial order=" << order;
    g_log.information(info.str());
    output = fitPolynomial(output, order, coeficients);
  }

  progress.report("CalculateTransmission: Performing fit");

  // if no rebin parameters were set the output workspace will have the same
  // binning as the input ones, otherwise rebin
  if (!rebinParams.empty()) {
    output = rebin(rebinParams, output);
  }
  progress.report("CalculateTransmission: Performing fit");

  // if there was rebinnning or log fitting we need to recalculate the Ys,
  // otherwise we can just use the workspace kicked out by the fitData()'s call
  // to Linear
  if ((!rebinParams.empty()) || logFit) {
    const MantidVec &X = output->readX(0);
    MantidVec &Y = output->dataY(0);
    if (logFit) {
      // Need to transform back to 'unlogged'
      const double m(std::pow(10, grad));
      const double factor(std::pow(10, offset));

      MantidVec &E = output->dataE(0);
      for (size_t i = 0; i < Y.size(); ++i) {
        // the relationship between the grad and interspt of the log fit and the
        // un-logged value of Y contain this dependence on the X (bin center
        // values)
        Y[i] = factor * (std::pow(m, 0.5 * (X[i] + X[i + 1])));
        E[i] = std::abs(E[i] * Y[i]);
        progress.report();
      }
    } // end logFit
    else if (fitMethod == "Linear") {
      // the simpler linear situation
      for (size_t i = 0; i < Y.size(); ++i) {
        Y[i] = (grad * 0.5 * (X[i] + X[i + 1])) + offset;
      }
    } else { // the polynomial fit
      for (size_t i = 0; i < Y.size(); ++i) {
        double aux = 0;
        double x_v = 0.5 * (X[i] + X[i + 1]);

        for (int j = 0; j < static_cast<int>(coeficients.size()); ++j) {
          aux += coeficients[j] * std::pow(x_v, j);
        }
        Y[i] = aux;
      }
    }
  }
  progress.report("CalculateTransmission: Performing fit");

  return output;
}
/** Uses 'Linear' as a ChildAlgorithm to fit the log of the exponential curve
 * expected for the transmission.
 *  @param[in] WS The single-spectrum workspace to fit
 *  @param[out] grad The single-spectrum workspace to fit
 *  @param[out] offset The single-spectrum workspace to fit
 *  @return A workspace containing the fit
 *  @throw runtime_error if the Linear algorithm fails during execution
 */
API::MatrixWorkspace_sptr
CalculateTransmission::fitData(API::MatrixWorkspace_sptr WS, double &grad,
                               double &offset) {
  g_log.information("Fitting the experimental transmission curve");
  double start = m_done;
  IAlgorithm_sptr childAlg = createChildAlgorithm("Fit", start, m_done = 0.9);
  auto linearBack =
      API::FunctionFactory::Instance().createFunction("LinearBackground");
  childAlg->setProperty("Function", linearBack);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  childAlg->setProperty("CreateOutput", true);
  childAlg->setProperty("IgnoreInvalidData", true);
  childAlg->executeAsChildAlg();

  std::string fitStatus = childAlg->getProperty("OutputStatus");
  if (fitStatus != "success") {
    g_log.error("Unable to successfully fit the data: " + fitStatus);
    throw std::runtime_error("Unable to successfully fit the data");
  }

  // Only get to here if successful
  offset = linearBack->getParameter(0);
  grad = linearBack->getParameter(1);
  return this->extractSpectrum(childAlg->getProperty("OutputWorkspace"), 1);
}
/** Uses Polynomial as a ChildAlgorithm to fit the log of the exponential curve
 * expected for the transmission.
 * @param[in] WS The single-spectrum workspace to fit
 * @param[in] order The order of the polynomial from 2 to 6
 * @param[out] coeficients of the polynomial. c[0] + c[1]x + c[2]x^2 + ...
 */
API::MatrixWorkspace_sptr
CalculateTransmission::fitPolynomial(API::MatrixWorkspace_sptr WS, int order,
                                     std::vector<double> &coeficients) {
  g_log.notice("Fitting the experimental transmission curve fitpolyno");
  double start = m_done;
  IAlgorithm_sptr childAlg = createChildAlgorithm("Fit", start, m_done = 0.9);
  auto polyfit = API::FunctionFactory::Instance().createFunction("Polynomial");
  polyfit->setAttributeValue("n", order);
  polyfit->initialize();
  childAlg->setProperty("Function", polyfit);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  childAlg->setProperty("CreateOutput", true);
  childAlg->setProperty("IgnoreInvalidData", true);
  childAlg->executeAsChildAlg();
  std::string fitStatus = childAlg->getProperty("OutputStatus");
  if (fitStatus != "success") {
    g_log.error("Unable to successfully fit the data: " + fitStatus);
    throw std::runtime_error("Unable to successfully fit the data");
  }

  // Only get to here if successful
  coeficients.resize(order + 1);
  for (int i = 0; i <= order; i++) {
    coeficients[i] = polyfit->getParameter(i);
  }
  return this->extractSpectrum(childAlg->getProperty("OutputWorkspace"), 1);
}

/** Calls rebin as Child Algorithm
*  @param binParams this string is passed to rebin as the "Params" property
*  @param ws the workspace to rebin
*  @return the resultant rebinned workspace
*  @throw runtime_error if the rebin algorithm fails during execution
*/
API::MatrixWorkspace_sptr
CalculateTransmission::rebin(std::vector<double> &binParams,
                             API::MatrixWorkspace_sptr ws) {
  double start = m_done;
  IAlgorithm_sptr childAlg =
      createChildAlgorithm("Rebin", start, m_done += 0.05);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
  childAlg->setProperty<std::vector<double>>("Params", binParams);
  childAlg->executeAsChildAlg();

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Algorithm
} // namespace Mantid
