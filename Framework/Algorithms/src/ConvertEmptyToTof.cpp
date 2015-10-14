//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertEmptyToTof.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BoundedValidator.h"

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

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertEmptyToTof::ConvertEmptyToTof() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertEmptyToTof::~ConvertEmptyToTof() {}

//----------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertEmptyToTof::init() {

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Empty");
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Name of the input workspace");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace, can be the same as the input");
  declareProperty(new Kernel::ArrayProperty<int>("ListOfSpectraIndices"),
                  "A list of spectra indices as a string with ranges; e.g. "
                  "5-10,15,20-23. \n"
                  "Optional: if not specified, then the Start/EndIndex fields "
                  "are used alone. "
                  "If specified, the range and the list are combined (without "
                  "duplicating indices). For example, a range of 10 to 20 and "
                  "a list '12,15,26,28' gives '10-20,26,28'.");

  declareProperty(new Kernel::ArrayProperty<int>("ListOfChannelIndices"),
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
                  "Spectrum index used for elastic peak position above.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertEmptyToTof::exec() {

  m_inputWS = this->getProperty("InputWorkspace");
  m_outputWS = this->getProperty("OutputWorkspace");
  std::vector<int> spectraIndices = getProperty("ListOfSpectraIndices");
  std::vector<int> channelIndices = getProperty("ListOfChannelIndices");
  int epp = getProperty("ElasticPeakPosition");
  int eppSpectrum = getProperty("ElasticPeakPositionSpectrum");

  std::vector<double> tofAxis;
  double channelWidth = getPropertyFromRun<double>(m_inputWS, "channel_width");

  // If the ElasticPeakPosition and the ElasticPeakPositionSpectrum were
  // specified
  if (epp != EMPTY_INT() && eppSpectrum != EMPTY_INT()) {
    g_log.information("Using the specified ElasticPeakPosition and "
                      "ElasticPeakPositionSpectrum");

    double wavelength = getPropertyFromRun<double>(m_inputWS, "wavelength");
    double l1 = getL1(m_inputWS);
    double l2 = getL2(m_inputWS, eppSpectrum);
    double epTof =
        (calculateTOF(l1, wavelength) + calculateTOF(l2, wavelength)) *
        1e6; // microsecs

    tofAxis = makeTofAxis(epp, epTof, m_inputWS->blocksize() + 1, channelWidth);

  }
  // If the spectraIndices and channelIndices were specified
  else {

    // validations
    validateSpectraIndices(spectraIndices);
    validateChannelIndices(channelIndices);

    // Map of spectra index, epp
    std::map<int, int> eppMap =
        findElasticPeakPositions(spectraIndices, channelIndices);

    for (auto it = eppMap.begin(); it != eppMap.end(); ++it) {
      g_log.debug() << "Spectra idx =" << it->first << ", epp=" << it->second
                    << std::endl;
    }

    std::pair<int, double> eppAndEpTof = findAverageEppAndEpTof(eppMap);

    tofAxis = makeTofAxis(eppAndEpTof.first, eppAndEpTof.second,
                          m_inputWS->blocksize() + 1, channelWidth);
  }

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != m_inputWS) {
    m_outputWS = API::WorkspaceFactory::Instance().create(m_inputWS);
  }

  setTofInWS(tofAxis, m_outputWS);

  setProperty("OutputWorkspace", m_outputWS);
}

/**
 * Check if spectra indices are in the limits of the number of histograms
 * in the input workspace. If v is empty, uses all spectra.
 * @param v :: vector with the spectra indices
 */
void ConvertEmptyToTof::validateSpectraIndices(std::vector<int> &v) {
  auto nHist = m_inputWS->getNumberHistograms();
  if (v.size() == 0) {
    g_log.information("No spectrum index given. Using all spectra to calculate "
                      "the elastic peak.");
    // use all spectra indices
    v.reserve(nHist);
    for (unsigned int i = 0; i < nHist; ++i)
      v[i] = i;
  } else {
    for (auto it = v.begin(); it != v.end(); ++it) {
      if (*it < 0 || static_cast<size_t>(*it) >= nHist) {
        throw std::runtime_error("Spectra index out of limits: " +
                                 boost::lexical_cast<std::string>(*it));
      }
    }
  }
}

/**
 * Check if the channel indices are in the limits of the number of the block
 * size
 * in the input workspace. If v is empty, uses all channels.
 * @param v :: vector with the channel indices to use
 */
void ConvertEmptyToTof::validateChannelIndices(std::vector<int> &v) {
  auto blockSize = m_inputWS->blocksize() + 1;
  if (v.size() == 0) {
    g_log.information("No channel index given. Using all channels (full "
                      "spectrum!) to calculate the elastic peak.");
    // use all channel indices
    v.reserve(blockSize);
    for (unsigned int i = 0; i < blockSize; ++i)
      v[i] = i;
  } else {
    for (auto it = v.begin(); it != v.end(); ++it) {
      if (*it < 0 || static_cast<size_t>(*it) >= blockSize) {
        throw std::runtime_error("Channel index out of limits: " +
                                 boost::lexical_cast<std::string>(*it));
      }
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

  g_log.information() << "Peak detection, search for peak " << std::endl;

  for (auto it = spectraIndices.begin(); it != spectraIndices.end(); ++it) {

    int spectrumIndex = *it;
    const Mantid::MantidVec &thisSpecY = m_inputWS->dataY(spectrumIndex);

    int minChannelIndex = *(channelIndices.begin());
    int maxChannelIndex = *(channelIndices.end() - 1);

    double center, sigma, height, minX, maxX;
    minX = static_cast<double>(minChannelIndex);
    maxX = static_cast<double>(maxChannelIndex);
    estimateFWHM(thisSpecY, center, sigma, height, minX, maxX);

    g_log.debug() << "Peak estimate :: center=" << center
                  << "\t sigma=" << sigma << "\t height=" << height
                  << "\t minX=" << minX << "\t maxX=" << maxX << std::endl;

    bool doFit =
        doFitGaussianPeak(spectrumIndex, center, sigma, height, minX, maxX);
    if (!doFit) {
      g_log.error() << "doFitGaussianPeak failed..." << std::endl;
      throw std::runtime_error("Gaussin Peak Fit failed....");
    }

    g_log.debug() << "Peak Fitting :: center=" << center << "\t sigma=" << sigma
                  << "\t height=" << height << "\t minX=" << minX
                  << "\t maxX=" << maxX << std::endl;

    // round up the center to the closest int
    eppMap[spectrumIndex] = roundUp(center);
  }
  return eppMap;
}

/**
 * Estimated the FWHM for Gaussian peak fitting
 *
 */
void ConvertEmptyToTof::estimateFWHM(const Mantid::MantidVec &spec,
                                     double &center, double &sigma,
                                     double &height, double &minX,
                                     double &maxX) {

  auto maxValueIt =
      std::max_element(spec.begin() + static_cast<size_t>(minX),
                       spec.begin() + static_cast<size_t>(maxX)); // max value
  double maxValue = *maxValueIt;
  size_t maxIndex =
      std::distance(spec.begin(), maxValueIt); // index of max value

  // indices and values for the fwhm detection
  size_t minFwhmIndex = maxIndex;
  size_t maxFwhmIndex = maxIndex;
  double minFwhmValue = maxValue;
  double maxFwhmValue = maxValue;
  // fwhm detection
  for (; minFwhmValue > 0.5 * maxValue;
       minFwhmIndex--, minFwhmValue = spec[minFwhmIndex]) {
  }
  for (; maxFwhmValue > 0.5 * maxValue;
       maxFwhmIndex++, maxFwhmValue = spec[maxFwhmIndex]) {
  }
  // double fwhm = thisSpecX[maxFwhmIndex] - thisSpecX[minFwhmIndex + 1];
  double fwhm = static_cast<double>(maxFwhmIndex - minFwhmIndex + 1);

  // parameters for the gaussian peak fit
  center = static_cast<double>(maxIndex);
  sigma = fwhm;
  height = maxValue;

  g_log.debug() << "Peak estimate  : center=" << center << "\t sigma=" << sigma
                << "\t h=" << height << std::endl;

  // determination of the range used for the peak definition
  size_t ipeak_min = std::max(
      static_cast<size_t>(0),
      maxIndex - static_cast<size_t>(
                     2.5 * static_cast<double>(maxIndex - maxFwhmIndex)));
  size_t ipeak_max = std::min(
      spec.size(),
      maxIndex + static_cast<size_t>(
                     2.5 * static_cast<double>(maxFwhmIndex - maxIndex)));
  size_t i_delta_peak = ipeak_max - ipeak_min;

  g_log.debug() << "Peak estimate xmin/max: " << ipeak_min - 1 << "\t"
                << ipeak_max + 1 << std::endl;

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
  auto *centerbound = API::ConstraintFactory::Instance().createInitialized(
      gaussianpeak.get(), os.str(), false);
  gaussianpeak->addConstraint(centerbound);

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
                    << gaussianpeak->centre() << std::endl;
    return false;
  }

  // 6. Get result
  center = gaussianpeak->centre();
  height = gaussianpeak->height();
  double fwhm = gaussianpeak->fwhm();
  if (fwhm <= 0.0) {
    return false;
  }
  //    sigma = fwhm*2;
  //  sigma = fwhm/2.35;

  return true;
}

/**
 * Finds the TOF for a given epp
 * @param eppMap : pair workspace spec index - epp
 * @return the average EPP and the corresponding average EP in TOF
 */

std::pair<int, double>
ConvertEmptyToTof::findAverageEppAndEpTof(const std::map<int, int> &eppMap) {

  double l1 = getL1(m_inputWS);
  double wavelength = getPropertyFromRun<double>(m_inputWS, "wavelength");

  std::vector<double> epTofList;
  std::vector<int> eppList;

  double firstL2 = getL2(m_inputWS, eppMap.begin()->first);
  for (auto it = eppMap.begin(); it != eppMap.end(); ++it) {

    double l2 = getL2(m_inputWS, it->first);
    if (!areEqual(l2, firstL2, 0.0001)) {
      g_log.error() << "firstL2=" << firstL2 << " , "
                    << "l2=" << l2 << std::endl;
      throw std::runtime_error("All the pixels for selected spectra must have "
                               "the same distance from the sample!");
    } else {
      firstL2 = l2;
    }

    epTofList.push_back(
        (calculateTOF(l1, wavelength) + calculateTOF(l2, wavelength)) *
        1e6); // microsecs
    eppList.push_back(it->first);

    g_log.debug() << "WS index = " << it->first << ", l1 = " << l1
                  << ", l2 = " << l2
                  << ", TOF(l1+l2) = " << *(epTofList.end() - 1) << std::endl;
  }

  double averageEpTof =
      std::accumulate(epTofList.begin(), epTofList.end(), 0.0) /
      static_cast<double>(epTofList.size());
  int averageEpp = roundUp(
      static_cast<double>(std::accumulate(eppList.begin(), eppList.end(), 0)) /
      static_cast<double>(eppList.size()));

  g_log.debug() << "Average epp=" << averageEpp
                << " , Average epTof=" << averageEpTof << std::endl;
  return std::make_pair(averageEpp, averageEpTof);
}

double ConvertEmptyToTof::getL1(API::MatrixWorkspace_const_sptr workspace) {
  Geometry::Instrument_const_sptr instrument = workspace->getInstrument();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  double l1 = instrument->getSource()->getDistance(*sample);
  return l1;
}

double ConvertEmptyToTof::getL2(API::MatrixWorkspace_const_sptr workspace,
                                int detId) {
  // Get a pointer to the instrument contained in the workspace
  Geometry::Instrument_const_sptr instrument = workspace->getInstrument();
  // Get the distance between the source and the sample (assume in metres)
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  // Get the sample-detector distance for this detector (in metres)
  double l2 =
      workspace->getDetector(detId)->getPos().distance(sample->getPos());
  return l2;
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

template <typename T>
T ConvertEmptyToTof::getPropertyFromRun(API::MatrixWorkspace_const_sptr inputWS,
                                        const std::string &propertyName) {
  if (inputWS->run().hasProperty(propertyName)) {
    Kernel::Property *prop = inputWS->run().getProperty(propertyName);
    return boost::lexical_cast<T>(prop->value());
  } else {
    std::string mesg =
        "No '" + propertyName + "' property found in the input workspace....";
    throw std::runtime_error(mesg);
  }
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
                << ", Channel Width=" << channelWidth << std::endl;
  for (size_t i = 0; i < size; ++i) {
    axis[i] =
        epTof + channelWidth * static_cast<double>(static_cast<int>(i) - epp) -
        channelWidth /
            2; // to make sure the bin is in the middle of the elastic peak
  }
  g_log.debug() << "TOF X Axis: [start,end] = [" << *axis.begin() << ","
                << *(axis.end() - 1) << "]" << std::endl;
  return axis;
}

void ConvertEmptyToTof::setTofInWS(const std::vector<double> &tofAxis,
                                   API::MatrixWorkspace_sptr outputWS) {

  const size_t numberOfSpectra = m_inputWS->getNumberHistograms();
  int64_t numberOfSpectraInt64 =
      static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

  g_log.debug() << "Setting the TOF X Axis for numberOfSpectra="
                << numberOfSpectra << std::endl;

  Progress prog(this, 0.0, 0.2, numberOfSpectra);
  PARALLEL_FOR2(m_inputWS, outputWS)
  for (int64_t i = 0; i < numberOfSpectraInt64; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Just copy over
    outputWS->dataY(i) = m_inputWS->readY(i);
    outputWS->dataE(i) = m_inputWS->readE(i);
    // copy
    outputWS->setX(i, tofAxis);

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
}

} // namespace Algorithms
} // namespace Mantid
