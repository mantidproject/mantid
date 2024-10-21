// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <utility>

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;

namespace // anonymous
{
// For LOQ at least, the transmission monitor is 3.  (The incident beam
// monitor's UDET is 2.)
const detid_t LOQ_TRANSMISSION_MONITOR_UDET = 3;

/**
 * Helper function to convert a single detector ID to a workspace index.
 * Should we just go ahead and add this to the MatrixWorkspace class?
 *
 * @param ws    :: workspace containing det ID to ws index mapping
 * @param detID :: the detector ID to look for
 *
 * @returns workspace index corresponding to the given detector ID
 */
size_t getIndexFromDetectorID(const MatrixWorkspace &ws, detid_t detid) {
  const std::vector<detid_t> input = {detid};
  std::vector<size_t> result = ws.getIndicesFromDetectorIDs(input);
  if (result.empty())
    throw std::invalid_argument("Could not find the spectra corresponding to detector ID " + std::to_string(detid));

  return result[0];
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateTransmission)

void CalculateTransmission::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<CommonBinsValidator>();
  wsValidator->add<HistogramValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("SampleRunWorkspace", "", Direction::Input, wsValidator),
                  "The workspace containing the sample transmission run. Must "
                  "have common binning and be in units of wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("DirectRunWorkspace", "", Direction::Input, wsValidator),
                  "The workspace containing the direct beam (no sample) "
                  "transmission run. The units and binning must match those of "
                  "the SampleRunWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace in which to store the fitted transmission "
                  "fractions.");

  auto zeroOrMore = std::make_shared<BoundedValidator<int>>();
  zeroOrMore->setLower(0);

  declareProperty("IncidentBeamMonitor", EMPTY_INT(), zeroOrMore, "The UDET of the incident beam monitor");
  declareProperty("TransmissionMonitor", EMPTY_INT(), zeroOrMore, "The UDET of the transmission monitor");

  declareProperty(std::make_unique<ArrayProperty<double>>("RebinParams"),
                  "A comma separated list of first bin boundary, width, last "
                  "bin boundary. Optionally\n"
                  "this can be followed by a comma and more widths and last "
                  "boundary pairs.\n"
                  "Negative width values indicate logarithmic binning.");

  std::vector<std::string> options(3);
  options[0] = "Linear";
  options[1] = "Log";
  options[2] = "Polynomial";

  declareProperty("FitMethod", "Log", std::make_shared<StringListValidator>(options),
                  "Whether to fit directly to the transmission curve using "
                  "Linear, Log or Polynomial.");
  auto twoOrMore = std::make_shared<BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("PolynomialOrder", 2, twoOrMore,
                  "Order of the polynomial to "
                  "fit. It is considered only "
                  "for FitMethod=Polynomial");

  declareProperty("OutputUnfittedData", false,
                  "If True, will output an additional workspace called "
                  "[OutputWorkspace]_unfitted containing the unfitted "
                  "transmission correction.");

  declareProperty(std::make_unique<ArrayProperty<detid_t>>("TransmissionROI"),
                  "An optional ArrayProperty containing a list of detector "
                  "ID's.  These specify a region of interest "
                  "which is to be summed and then used instead of a "
                  "transmission monitor. This allows for a \"beam stop "
                  "out\" method of transmission calculation.");
}

void CalculateTransmission::exec() {
  m_done = 0.;
  MatrixWorkspace_sptr sampleWS = getProperty("SampleRunWorkspace");
  MatrixWorkspace_sptr directWS = getProperty("DirectRunWorkspace");

  const detid_t beamMonitorID = getProperty("IncidentBeamMonitor");
  detid_t transMonitorID = getProperty("TransmissionMonitor");
  const std::vector<detid_t> transDetList = getProperty("TransmissionROI");

  const bool usingSameInstrument = sampleWS->getInstrument()->getName() == directWS->getInstrument()->getName();
  if (!usingSameInstrument)
    throw std::invalid_argument("The input workspaces do not come from the same instrument.");
  if (!WorkspaceHelpers::matchingBins(*sampleWS, *directWS))
    throw std::invalid_argument("The input workspaces do not have matching bins.");

  bool usingMonitor = !isEmpty(transMonitorID);
  const bool usingROI = !transDetList.empty();
  if (usingMonitor && usingROI)
    throw std::invalid_argument("Unable to use both a monitor and a region of "
                                "interest in transmission calculation.");
  if (!usingMonitor && !usingROI) {
    transMonitorID = LOQ_TRANSMISSION_MONITOR_UDET;
    usingMonitor = true;
  }

  // Populate transmissionIndices with the workspace indices to use for the
  // transmission.
  // In the case of TransmissionMonitor this will be a single index
  // corresponding to a
  // monitor, in the case of TransmissionROI it will be one or more indices
  // corresponding
  // to a region of interest on the detector bank(s).
  std::vector<size_t> transmissionIndices;
  if (usingMonitor) {
    const size_t transmissionMonitorIndex = getIndexFromDetectorID(*sampleWS, transMonitorID);
    transmissionIndices.emplace_back(transmissionMonitorIndex);
    logIfNotMonitor(sampleWS, directWS, transmissionMonitorIndex);
  } else if (usingROI) {
    transmissionIndices = sampleWS->getIndicesFromDetectorIDs(transDetList);
  } else
    assert(false);

  const std::string transPropName = usingMonitor ? "TransmissionMonitor" : "TransmissionROI";

  if (transmissionIndices.empty())
    throw std::invalid_argument("The UDET(s) passed to " + transPropName +
                                " do not correspond to spectra in the workspace.");

  // Check if we're normalising to the incident beam monitor.  If so, then it
  // needs to be a monitor that is not also used for the transmission.
  const bool normaliseToMonitor = !isEmpty(beamMonitorID);
  size_t beamMonitorIndex = 0;
  if (normaliseToMonitor) {
    beamMonitorIndex = getIndexFromDetectorID(*sampleWS, beamMonitorID);
    logIfNotMonitor(sampleWS, directWS, beamMonitorIndex);

    const auto transmissionIndex = std::find(transmissionIndices.begin(), transmissionIndices.end(), beamMonitorIndex);
    if (transmissionIndex != transmissionIndices.end())
      throw std::invalid_argument("The IncidentBeamMonitor UDET (" + std::to_string(*transmissionIndex) +
                                  ") matches a UDET given in " + transPropName + ".");
  }

  MatrixWorkspace_sptr sampleInc;
  if (normaliseToMonitor)
    sampleInc = this->extractSpectra(sampleWS, std::vector<size_t>(1, beamMonitorIndex));
  MatrixWorkspace_sptr sampleTrans = this->extractSpectra(sampleWS, transmissionIndices);

  MatrixWorkspace_sptr directInc;
  if (normaliseToMonitor)
    directInc = this->extractSpectra(directWS, std::vector<size_t>(1, beamMonitorIndex));
  MatrixWorkspace_sptr directTrans = this->extractSpectra(directWS, transmissionIndices);

  double start = m_done;
  Progress progress(this, start, m_done += 0.2, 2);
  progress.report("CalculateTransmission: Dividing transmission by incident");

  // The main calculation
  MatrixWorkspace_sptr transmission = sampleTrans / directTrans;
  if (normaliseToMonitor)
    transmission = transmission * (directInc / sampleInc);

  // This workspace is now a distribution
  progress.report("CalculateTransmission: Dividing transmission by incident");

  // Output this data if requested
  const bool outputRaw = getProperty("OutputUnfittedData");
  if (outputRaw) {
    auto childAlg = createChildAlgorithm("ReplaceSpecialValues");
    childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", transmission);
    childAlg->setProperty<double>("NaNValue", 0.0);
    childAlg->setProperty<double>("NaNError", 0.0);
    childAlg->setProperty<double>("InfinityValue", 0.0);
    childAlg->setProperty<double>("InfinityError", 0.0);
    childAlg->executeAsChildAlg();
    transmission = childAlg->getProperty("OutputWorkspace");
    std::string outputWSName = getPropertyValue("OutputWorkspace");
    outputWSName += "_unfitted";
    declareProperty(std::make_unique<WorkspaceProperty<>>("UnfittedData", outputWSName, Direction::Output));
    setProperty("UnfittedData", transmission);
  }

  // Check that there are more than a single bin in the transmission
  // workspace. Skip the fit if there isn't.
  if (transmission->y(0).size() > 1) {
    transmission = fit(transmission, getProperty("RebinParams"), getProperty("FitMethod"));
  }
  setProperty("OutputWorkspace", transmission);
}

/**
 * Extracts multiple spectra from a Workspace2D into a new workspaces, using
 *SumSpectra.
 *
 * @param ws      :: The workspace containing the spectrum to extract
 * @param indices :: The workspace index of the spectrum to extract
 *
 * @returns a Workspace2D containing the extracted spectrum
 * @throws runtime_error if the ExtractSingleSpectrum algorithm fails during
 *execution
 */
API::MatrixWorkspace_sptr CalculateTransmission::extractSpectra(const API::MatrixWorkspace_sptr &ws,
                                                                const std::vector<size_t> &indices) {
  // Compile a comma separated list of indices that we can pass to SumSpectra.
  std::vector<std::string> indexStrings(indices.size());
  // A bug in boost 1.53: https://svn.boost.org/trac/boost/ticket/7421
  // means that lexical_cast cannot be used directly as the call is ambiguous
  // so we need to define a function pointer that can resolve the overloaded
  // lexical_cast function
  using from_size_t = std::string (*)(const size_t &);

  std::transform(indices.begin(), indices.end(), indexStrings.begin(),
                 static_cast<from_size_t>(boost::lexical_cast<std::string, size_t>));
  const std::string commaIndexList = boost::algorithm::join(indexStrings, ",");

  double start = m_done;
  auto childAlg = createChildAlgorithm("SumSpectra", start, m_done += 0.1);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
  childAlg->setPropertyValue("ListOfWorkspaceIndices", commaIndexList);
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
API::MatrixWorkspace_sptr CalculateTransmission::fit(const API::MatrixWorkspace_sptr &raw,
                                                     const std::vector<double> &rebinParams,
                                                     const std::string &fitMethod) {
  MatrixWorkspace_sptr output = this->extractSpectra(raw, std::vector<size_t>(1, 0));

  Progress progress(this, m_done, 1.0, 4);
  progress.report("CalculateTransmission: Performing fit");

  // these are calculated by the call to fit below
  double grad(0.0), offset(0.0);
  std::vector<double> coeficients;
  const bool logFit = (fitMethod == "Log");
  if (logFit) {
    g_log.debug("Fitting to the logarithm of the transmission");

    auto &Y = output->mutableY(0);
    auto &E = output->mutableE(0);
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
  } // logFit true
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
    auto &X = output->x(0);
    auto &Y = output->mutableY(0);
    if (logFit) {
      // Need to transform back to 'unlogged'
      const double m(std::pow(10, grad));
      const double factor(std::pow(10, offset));

      auto &E = output->mutableE(0);
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
API::MatrixWorkspace_sptr CalculateTransmission::fitData(const API::MatrixWorkspace_sptr &WS, double &grad,
                                                         double &offset) {
  g_log.information("Fitting the experimental transmission curve");
  double start = m_done;
  auto childAlg = createChildAlgorithm("Fit", start, m_done + 0.9);
  auto linearBack = API::FunctionFactory::Instance().createFunction("LinearBackground");
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
  return this->extractSpectra(childAlg->getProperty("OutputWorkspace"), std::vector<size_t>(1, 1));
}
/** Uses Polynomial as a ChildAlgorithm to fit the log of the exponential curve
 * expected for the transmission.
 * @param[in] WS The single-spectrum workspace to fit
 * @param[in] order The order of the polynomial from 2 to 6
 * @param[out] coeficients of the polynomial. c[0] + c[1]x + c[2]x^2 + ...
 */
API::MatrixWorkspace_sptr CalculateTransmission::fitPolynomial(const API::MatrixWorkspace_sptr &WS, int order,
                                                               std::vector<double> &coeficients) {
  g_log.notice("Fitting the experimental transmission curve fitpolyno");
  double start = m_done;
  auto childAlg = createChildAlgorithm("Fit", start, m_done = 0.9);
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
  return this->extractSpectra(childAlg->getProperty("OutputWorkspace"), std::vector<size_t>(1, 1));
}

/** Calls rebin as Child Algorithm
 *  @param binParams this string is passed to rebin as the "Params" property
 *  @param ws the workspace to rebin
 *  @return the resultant rebinned workspace
 *  @throw runtime_error if the rebin algorithm fails during execution
 */
API::MatrixWorkspace_sptr CalculateTransmission::rebin(const std::vector<double> &binParams,
                                                       const API::MatrixWorkspace_sptr &ws) {
  double start = m_done;
  auto childAlg = createChildAlgorithm("Rebin", start, m_done += 0.05);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
  childAlg->setProperty<std::vector<double>>("Params", binParams);
  childAlg->executeAsChildAlg();

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

/**
 * Outputs message to log if the detector at the given index is not a monitor in
 *both input workspaces.
 *
 * @param sampleWS :: the input sample workspace
 * @param directWS :: the input direct workspace
 * @param index    :: the index of the detector to checked
 */
void CalculateTransmission::logIfNotMonitor(const API::MatrixWorkspace_sptr &sampleWS,
                                            const API::MatrixWorkspace_sptr &directWS, size_t index) {
  const std::string message = "The detector at index " + std::to_string(index) + " is not a monitor in the ";
  if (!sampleWS->spectrumInfo().isMonitor(index))
    g_log.information(message + "sample workspace.");
  if (!directWS->spectrumInfo().isMonitor(index))
    g_log.information(message + "direct workspace.");
}

} // namespace Mantid::Algorithms
