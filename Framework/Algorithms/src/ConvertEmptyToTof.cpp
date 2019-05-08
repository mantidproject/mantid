// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertEmptyToTof.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <map>
#include <numeric> // std::accumulate
#include <utility> // std::pair

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertEmptyToTof)

/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertEmptyToTof::name() const {
  return "ConvertEmptyToTof";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvertEmptyToTof::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertEmptyToTof::category() const {
  return "Transforms\\Units";
}

/** Initialize the algorithm's properties.
 */
void ConvertEmptyToTof::init() {

  auto wsValidator = boost::make_shared<WorkspaceUnitValidator>("Empty");
  declareProperty(make_unique<WorkspaceProperty<DataObjects::Workspace2D>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Name of the input workspace");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace, can be the same as the input");
  declareProperty(
      make_unique<Kernel::ArrayProperty<int>>("ListOfSpectraIndices"),
      "A list of spectra workspace indices as a string with ranges; e.g. "
      "5-10,15,20-23. \n"
      "Optional: if not specified, then the Start/EndIndex fields "
      "are used alone. "
      "If specified, the range and the list are combined (without "
      "duplicating indices). For example, a range of 10 to 20 and "
      "a list '12,15,26,28' gives '10-20,26,28'.");

  declareProperty(
      make_unique<Kernel::ArrayProperty<int>>("ListOfChannelIndices"),
      "A list of spectra indices as a string with ranges; e.g. "
      "5-10,15,20-23. \n"
      "Optional: if not specified, then the Start/EndIndex fields "
      "are used alone. "
      "If specified, the range and the list are combined (without "
      "duplicating indices). For example, a range of 10 to 20 and "
      "a list '12,15,26,28' gives '10-20,26,28'.");

  // OR Specify EPP
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "ElasticPeakPosition", EMPTY_INT(), mustBePositive,
      "Value of elastic peak position if none of the above are filled in.");
  declareProperty("ElasticPeakPositionSpectrum", EMPTY_INT(), mustBePositive,
                  "Workspace Index used for elastic peak position above.");
}

/** Execute the algorithm.
 */
void ConvertEmptyToTof::exec() {

  m_inputWS = this->getProperty("InputWorkspace");
  m_outputWS = this->getProperty("OutputWorkspace");
  m_spectrumInfo = &m_inputWS->spectrumInfo();
  std::vector<int> spectraIndices = getProperty("ListOfSpectraIndices");
  std::vector<int> channelIndices = getProperty("ListOfChannelIndices");
  const int epp = getProperty("ElasticPeakPosition");
  const int eppSpectrum = getProperty("ElasticPeakPositionSpectrum");

  std::vector<double> tofAxis;
  double channelWidth{0.0};
  if (m_inputWS->run().hasProperty("channel_width")) {
    channelWidth =
        m_inputWS->run().getPropertyValueAsType<double>("channel_width");
  } else {
    const std::string mesg =
        "No property channel_width found in the input workspace....";
    throw std::runtime_error(mesg);
  }

  // If the ElasticPeakPosition and the ElasticPeakPositionSpectrum were
  // specified
  if (epp != EMPTY_INT() && eppSpectrum != EMPTY_INT()) {
    g_log.information("Using the specified ElasticPeakPosition and "
                      "ElasticPeakPositionSpectrum");

    double wavelength{0.0};
    if (m_inputWS->run().hasProperty("wavelength")) {
      wavelength =
          m_inputWS->run().getPropertyValueAsType<double>("wavelength");
    } else {
      std::string mesg =
          "No property wavelength found in the input workspace....";
      throw std::runtime_error(mesg);
    }

    const double l1 = m_spectrumInfo->l1();
    const double l2 = m_spectrumInfo->l2(eppSpectrum);
    const double epTof =
        (calculateTOF(l1, wavelength) + calculateTOF(l2, wavelength)) *
        1e6; // microsecs

    tofAxis = makeTofAxis(epp, epTof, m_inputWS->blocksize() + 1, channelWidth);

  }
  // If the spectraIndices and channelIndices were specified
  else {

    // validations
    validateWorkspaceIndices(spectraIndices);
    validateChannelIndices(channelIndices);

    // Map of spectra index, epp
    const std::map<int, int> eppMap =
        findElasticPeakPositions(spectraIndices, channelIndices);

    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      for (auto &e : eppMap) {
        g_log.debug() << "Spectra idx =" << e.first << ", epp=" << e.second
                      << '\n';
      }
    }

    const std::pair<int, double> eppAndEpTof = findAverageEppAndEpTof(eppMap);

    tofAxis = makeTofAxis(eppAndEpTof.first, eppAndEpTof.second,
                          m_inputWS->blocksize() + 1, channelWidth);
  }

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != m_inputWS) {
    m_outputWS = m_inputWS->clone();
  }

  setTofInWS(tofAxis, m_outputWS);

  setProperty("OutputWorkspace", m_outputWS);
}

/**
 * Check if spectra indices are in the limits of the number of histograms
 * in the input workspace. If v is empty, uses all spectra.
 * @param v :: vector with the spectra indices
 */
void ConvertEmptyToTof::validateWorkspaceIndices(std::vector<int> &v) {
  auto nHist = m_inputWS->getNumberHistograms();
  if (v.empty()) {
    g_log.information(
        "No spectrum number given. Using all spectra to calculate "
        "the elastic peak.");
    // use all spectra indices
    v.reserve(nHist);

    for (unsigned int i = 0; i < nHist; ++i)
      v[i] = i;
  } else {
    const auto found =
        std::find_if(v.cbegin(), v.cend(), [nHist](const auto index) {
          return index < 0 || static_cast<size_t>(index) >= nHist;
        });
    if (found != v.cend()) {
      throw std::runtime_error("Spectra index out of limits: " +
                               std::to_string(*found));
    }
  }
}

/**
 * Check if the channel indices are in the limits of the number of the block
 * size in the input workspace. If v is empty, uses all channels.
 * @param v :: vector with the channel indices to use
 */
void ConvertEmptyToTof::validateChannelIndices(std::vector<int> &v) {
  auto blockSize = m_inputWS->blocksize() + 1;
  if (v.empty()) {
    g_log.information("No channel index given. Using all channels (full "
                      "spectrum!) to calculate the elastic peak.");
    // use all channel indices
    v.reserve(blockSize);
    for (unsigned int i = 0; i < blockSize; ++i)
      v.emplace_back(i);
  } else {
    const auto found =
        std::find_if(v.cbegin(), v.cend(), [blockSize](const auto index) {
          return index < 0 || static_cast<size_t>(index) >= blockSize;
        });
    if (found != v.cend()) {
      throw std::runtime_error("Channel index out of limits: " +
                               std::to_string(*found));
    }
  }
}

/**
 * Looks for the EPP positions in the spectraIndices
 * @return map with worskpace spectra index, elastic peak position for this
 * spectra
 */
std::map<int, int> ConvertEmptyToTof::findElasticPeakPositions(
    const std::vector<int> &spectraIndices,
    const std::vector<int> &channelIndices) {

  std::map<int, int> eppMap;

  // make sure we not looking for channel indices outside the bounds
  assert(static_cast<size_t>(*(channelIndices.end() - 1)) <
         m_inputWS->blocksize() + 1);

  g_log.information() << "Peak detection, search for peak \n";

  for (auto spectrumIndex : spectraIndices) {

    auto &thisSpecY = m_inputWS->y(spectrumIndex);

    int minChannelIndex = *(channelIndices.begin());
    int maxChannelIndex = *(channelIndices.end() - 1);

    double center, sigma, height, minX, maxX;
    minX = static_cast<double>(minChannelIndex);
    maxX = static_cast<double>(maxChannelIndex);
    estimateFWHM(thisSpecY, center, sigma, height, minX, maxX);

    g_log.debug() << "Peak estimate :: center=" << center
                  << "\t sigma=" << sigma << "\t height=" << height
                  << "\t minX=" << minX << "\t maxX=" << maxX << '\n';

    bool doFit =
        doFitGaussianPeak(spectrumIndex, center, sigma, height, minX, maxX);
    if (!doFit) {
      g_log.error() << "doFitGaussianPeak failed...\n";
      throw std::runtime_error("Gaussin Peak Fit failed....");
    }

    g_log.debug() << "Peak Fitting :: center=" << center << "\t sigma=" << sigma
                  << "\t height=" << height << "\t minX=" << minX
                  << "\t maxX=" << maxX << '\n';

    // round up the center to the closest int
    eppMap[spectrumIndex] = roundUp(center);
  }
  return eppMap;
}

/**
 * Estimated the FWHM for Gaussian peak fitting
 *
 */
void ConvertEmptyToTof::estimateFWHM(
    const Mantid::HistogramData::HistogramY &spec, double &center,
    double &sigma, double &height, double &minX, double &maxX) {

  auto maxValueIt =
      std::max_element(spec.begin() + static_cast<size_t>(minX),
                       spec.begin() + static_cast<size_t>(maxX)); // max value
  double maxValue = *maxValueIt;
  size_t maxIndex =
      std::distance(spec.begin(), maxValueIt); // index of max value

  auto minFwhmIt =
      std::find_if(MantidVec::const_reverse_iterator(maxValueIt),
                   MantidVec::const_reverse_iterator(spec.cbegin()),
                   [maxValue](double value) { return value < 0.5 * maxValue; });
  auto maxFwhmIt =
      std::find_if(maxValueIt, spec.end(),
                   [maxValue](double value) { return value < 0.5 * maxValue; });

  // double fwhm = thisSpecX[maxFwhmIndex] - thisSpecX[minFwhmIndex + 1];
  double fwhm =
      static_cast<double>(std::distance(minFwhmIt.base(), maxFwhmIt) + 1);

  // parameters for the gaussian peak fit
  center = static_cast<double>(maxIndex);
  sigma = fwhm;
  height = maxValue;

  g_log.debug() << "Peak estimate  : center=" << center << "\t sigma=" << sigma
                << "\t h=" << height << '\n';

  // determination of the range used for the peak definition
  size_t ipeak_min = std::max(
      std::size_t{0},
      maxIndex - static_cast<size_t>(2.5 * static_cast<double>(std::distance(
                                               maxValueIt, maxFwhmIt))));
  size_t ipeak_max = std::min(
      spec.size(),
      maxIndex + static_cast<size_t>(2.5 * static_cast<double>(std::distance(
                                               maxFwhmIt, maxValueIt))));
  size_t i_delta_peak = ipeak_max - ipeak_min;

  g_log.debug() << "Peak estimate xmin/max: " << ipeak_min - 1 << "\t"
                << ipeak_max + 1 << '\n';

  minX = static_cast<double>(ipeak_min - 2 * i_delta_peak);
  maxX = static_cast<double>(ipeak_max + 2 * i_delta_peak);
}

/**
 * Fit peak without background i.e, with background removed
 *  inspired from FitPowderDiffPeaks.cpp
 *  copied from PoldiPeakDetection2.cpp
 *
 @param workspaceindex :: indice of the row to use
 @param center :: gaussian parameter - center
 @param sigma :: gaussian parameter - width
 @param height :: gaussian parameter - height
 @param startX :: fit range - start X value
 @param endX :: fit range - end X value
 @returns A boolean status flag, true for fit success, false else
 */
bool ConvertEmptyToTof::doFitGaussianPeak(int workspaceindex, double &center,
                                          double &sigma, double &height,
                                          double startX, double endX) {

  g_log.debug("Calling doFitGaussianPeak...");

  // 1. Estimate
  sigma = sigma * 0.5;

  // 2. Use factory to generate Gaussian
  auto temppeak = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto gaussianpeak = boost::dynamic_pointer_cast<API::IPeakFunction>(temppeak);
  gaussianpeak->setHeight(height);
  gaussianpeak->setCentre(center);
  gaussianpeak->setFwhm(sigma);

  // 3. Constraint
  double centerleftend = center - sigma * 0.5;
  double centerrightend = center + sigma * 0.5;
  std::ostringstream os;
  os << centerleftend << " < PeakCentre < " << centerrightend;
  auto centerbound = std::unique_ptr<API::IConstraint>(
      API::ConstraintFactory::Instance().createInitialized(gaussianpeak.get(),
                                                           os.str(), false));
  gaussianpeak->addConstraint(std::move(centerbound));

  g_log.debug("Calling createChildAlgorithm : Fit...");
  // 4. Fit
  API::IAlgorithm_sptr fitalg = createChildAlgorithm("Fit", -1, -1, true);
  fitalg->initialize();

  fitalg->setProperty(
      "Function", boost::dynamic_pointer_cast<API::IFunction>(gaussianpeak));
  fitalg->setProperty("InputWorkspace", m_inputWS);
  fitalg->setProperty("WorkspaceIndex", workspaceindex);
  fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);
  fitalg->setProperty("Output", "FitGaussianPeak");
  fitalg->setProperty("StartX", startX);
  fitalg->setProperty("EndX", endX);

  // 5.  Result
  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || !successfulfit) {
    // Early return due to bad fit
    g_log.warning() << "Fitting Gaussian peak for peak around "
                    << gaussianpeak->centre() << '\n';
    return false;
  }

  // 6. Get result
  center = gaussianpeak->centre();
  height = gaussianpeak->height();
  double fwhm = gaussianpeak->fwhm();

  return fwhm > 0.0;
}

/**
 * Finds the TOF for a given epp
 * @param eppMap : pair workspace spec index - epp
 * @return the average EPP and the corresponding average EP in TOF
 */

std::pair<int, double>
ConvertEmptyToTof::findAverageEppAndEpTof(const std::map<int, int> &eppMap) {

  double l1 = m_spectrumInfo->l1();
  double wavelength{0.0};
  if (m_inputWS->run().hasProperty("wavelength")) {
    wavelength = m_inputWS->run().getPropertyValueAsType<double>("wavelength");
  } else {
    std::string mesg =
        "No property wavelength found in the input workspace....";
    throw std::runtime_error(mesg);
  }

  std::vector<double> epTofList;
  std::vector<int> eppList;

  double firstL2 = m_spectrumInfo->l2(eppMap.begin()->first);
  for (const auto &epp : eppMap) {

    double l2 = m_spectrumInfo->l2(epp.first);
    if (!areEqual(l2, firstL2, 0.0001)) {
      g_log.error() << "firstL2=" << firstL2 << " , "
                    << "l2=" << l2 << '\n';
      throw std::runtime_error("All the pixels for selected spectra must have "
                               "the same distance from the sample!");
    } else {
      firstL2 = l2;
    }

    epTofList.push_back(
        (calculateTOF(l1, wavelength) + calculateTOF(l2, wavelength)) *
        1e6); // microsecs
    eppList.push_back(epp.first);

    g_log.debug() << "WS index = " << epp.first << ", l1 = " << l1
                  << ", l2 = " << l2
                  << ", TOF(l1+l2) = " << *(epTofList.end() - 1) << '\n';
  }

  double averageEpTof =
      std::accumulate(epTofList.begin(), epTofList.end(), 0.0) /
      static_cast<double>(epTofList.size());
  int averageEpp = roundUp(
      static_cast<double>(std::accumulate(eppList.begin(), eppList.end(), 0)) /
      static_cast<double>(eppList.size()));

  g_log.debug() << "Average epp=" << averageEpp
                << " , Average epTof=" << averageEpTof << '\n';
  return std::make_pair(averageEpp, averageEpTof);
}

double ConvertEmptyToTof::calculateTOF(double distance, double wavelength) {
  if (wavelength <= 0) {
    throw std::runtime_error("Wavelenght is <= 0");
  }

  double velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass *
                                            wavelength * 1e-10); // m/s

  return distance / velocity;
}

/**
 * Compare two double with a precision epsilon
 */
bool ConvertEmptyToTof::areEqual(double a, double b, double epsilon) {
  return fabs(a - b) < epsilon;
}

int ConvertEmptyToTof::roundUp(double value) {
  return static_cast<int>(std::floor(value + 0.5));
}

/**
 * Builds the X time axis
 */
std::vector<double> ConvertEmptyToTof::makeTofAxis(int epp, double epTof,
                                                   size_t size,
                                                   double channelWidth) {
  std::vector<double> axis(size);

  g_log.debug() << "Building the TOF X Axis: epp=" << epp << ", epTof=" << epTof
                << ", Channel Width=" << channelWidth << '\n';
  for (size_t i = 0; i < size; ++i) {
    axis[i] =
        epTof + channelWidth * static_cast<double>(static_cast<int>(i) - epp) -
        channelWidth /
            2; // to make sure the bin is in the middle of the elastic peak
  }
  g_log.debug() << "TOF X Axis: [start,end] = [" << *axis.begin() << ","
                << *(axis.end() - 1) << "]\n";
  return axis;
}

void ConvertEmptyToTof::setTofInWS(const std::vector<double> &tofAxis,
                                   API::MatrixWorkspace_sptr outputWS) {

  const size_t numberOfSpectra = m_inputWS->getNumberHistograms();

  g_log.debug() << "Setting the TOF X Axis for numberOfSpectra="
                << numberOfSpectra << '\n';

  HistogramData::BinEdges edges(tofAxis);
  Progress prog(this, 0.0, 0.2, numberOfSpectra);

  for (size_t i = 0; i < numberOfSpectra; ++i) {
    // Replace bin edges with tof axis
    outputWS->setBinEdges(i, edges);

    prog.report();
  } // end for i

  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
}

} // namespace Algorithms
} // namespace Mantid
