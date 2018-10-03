// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GetEi.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/lexical_cast.hpp>
#include <cmath>
#include <numeric>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GetEi)

using namespace Kernel;
using namespace API;
using namespace Geometry;

// adjustable fit criteria, increase the first number or reduce any of the last
// three for more promiscuous peak fitting
// from the estimated location of the peak search forward by the following
// fraction and backward by the same fraction
const double GetEi::HALF_WINDOW = 8.0 / 100;
const double GetEi::PEAK_THRESH_H = 3.0;
const double GetEi::PEAK_THRESH_A = 5.0;
const int64_t GetEi::PEAK_THRESH_W = 3;

// progress estimates
const double GetEi::CROP = 0.15;
const double GetEi::GET_COUNT_RATE = 0.15;
const double GetEi::FIT_PEAK = 0.2;

/// Empty default constructor algorith() calls the constructor in the base class
GetEi::GetEi() : Algorithm(), m_tempWS(), m_fracCompl(0.0) {}

void GetEi::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto val = boost::make_shared<CompositeValidator>();
  val->add<WorkspaceUnitValidator>("TOF");
  val->add<HistogramValidator>();
  val->add<InstrumentValidator>();
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       val),
      "The X units of this workspace must be time of flight with times in\n"
      "micro-seconds");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "Monitor1Spec", -1, mustBePositive,
      "The spectrum number of the output of the first monitor, e.g. MAPS\n"
      "41474, MARI 2, MERLIN 69634");
  declareProperty(
      "Monitor2Spec", -1, mustBePositive,
      "The spectrum number of the output of the second monitor e.g. MAPS\n"
      "41475, MARI 3, MERLIN 69638");
  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty(
      "EnergyEstimate", -1.0, positiveDouble,
      "An approximate value for the typical incident energy, energy of\n"
      "neutrons leaving the source (meV)");
  declareProperty("IncidentEnergy", -1.0, Direction::Output);
  declareProperty("FirstMonitorPeak", -1.0, Direction::Output);

  m_fracCompl = 0.0;
}

/** Executes the algorithm
 *  @throw out_of_range if the peak runs off the edge of the histogram
 *  @throw NotFoundError if one of the requested spectrum numbers was not found
 * in the workspace
 *  @throw IndexError if there is a problem converting spectra indexes to
 * spectra numbers, which would imply there is a problem with the workspace
 *  @throw invalid_argument if a good peak fit wasn't made or the input
 * workspace does not have common binning
 */
void GetEi::exec() {
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const specnum_t mon1Spec = getProperty("Monitor1Spec");
  const specnum_t mon2Spec = getProperty("Monitor2Spec");
  double dist2moni0 = -1, dist2moni1 = -1;
  getGeometry(inWS, mon1Spec, mon2Spec, dist2moni0, dist2moni1);

  // the E_i estimate is used to find (identify) the monitor peaks, checking
  // prior to fitting will throw an exception if this estimate is too big or
  // small
  const double E_est = getProperty("EnergyEstimate");
  // we're assuming that the time units for the X-values in the workspace are
  // micro-seconds
  const double peakLoc0 = 1e6 * timeToFly(dist2moni0, E_est);
  // write a lot of stuff to the log at user level as it is very possible for
  // fit routines not to the expected thing
  g_log.information() << "Based on the user selected energy the first peak "
                         "will be searched for at TOF "
                      << peakLoc0 << " micro seconds +/-"
                      << boost::lexical_cast<std::string>(100.0 * HALF_WINDOW)
                      << "%\n";
  const double peakLoc1 = 1e6 * timeToFly(dist2moni1, E_est);
  g_log.information() << "Based on the user selected energy the second peak "
                         "will be searched for at TOF "
                      << peakLoc1 << " micro seconds +/-"
                      << boost::lexical_cast<std::string>(100.0 * HALF_WINDOW)
                      << "%\n";

  // get the histograms created by the monitors
  std::vector<size_t> indexes = getMonitorWsIndexs(inWS, mon1Spec, mon2Spec);

  g_log.information()
      << "Looking for a peak in the first monitor spectrum, spectra index "
      << indexes[0] << '\n';
  double t_monitor0 = getPeakCentre(inWS, indexes[0], peakLoc0);
  g_log.notice() << "The first peak has been found at TOF = " << t_monitor0
                 << " microseconds\n";
  setProperty("FirstMonitorPeak", t_monitor0);

  g_log.information()
      << "Looking for a peak in the second monitor spectrum, spectra index "
      << indexes[1] << '\n';
  double t_monitor1 = getPeakCentre(inWS, indexes[1], peakLoc1);
  g_log.information() << "The second peak has been found at TOF = "
                      << t_monitor1 << " microseconds\n";

  // assumes that the source and the both mintors lie on one straight line, the
  // 1e-6 converts microseconds to seconds as the mean speed needs to be in m/s
  double meanSpeed =
      (dist2moni1 - dist2moni0) / (1e-6 * (t_monitor1 - t_monitor0));

  // uses 0.5mv^2 to get the kinetic energy in joules which we then convert to
  // meV
  double E_i = neutron_E_At(meanSpeed) / PhysicalConstants::meV;
  g_log.notice() << "The incident energy has been calculated to be " << E_i
                 << " meV"
                 << " (your estimate was " << E_est << " meV)\n";

  setProperty("IncidentEnergy", E_i);
  // store property in input workspace
  Property *incident_energy =
      new PropertyWithValue<double>("Ei", E_i, Direction::Input);
  inWS->mutableRun().addProperty(incident_energy, true);
}
/** Gets the distances between the source and detectors whose IDs you pass to it
 *  @param WS :: the input workspace
 *  @param mon0Spec :: Spectrum number of the output from the first monitor
 *  @param mon1Spec :: Spectrum number of the output from the second monitor
 *  @param monitor0Dist :: the calculated distance to the detector whose ID was
 * passed to this function first
 *  @param monitor1Dist :: calculated distance to the detector whose ID was
 * passed to this function second
 *  @throw NotFoundError if no detector is found for the detector ID given
 */
void GetEi::getGeometry(API::MatrixWorkspace_const_sptr WS, specnum_t mon0Spec,
                        specnum_t mon1Spec, double &monitor0Dist,
                        double &monitor1Dist) const {
  const IComponent_const_sptr source = WS->getInstrument()->getSource();

  // retrieve a pointer to the first detector and get its distance
  size_t monWI = 0;
  try {
    monWI = WS->getIndexFromSpectrumNumber(mon0Spec);
  } catch (std::runtime_error &) {
    g_log.error()
        << "Could not find the workspace index for the monitor at spectrum "
        << mon0Spec << "\n";
    g_log.error() << "Error retrieving data for the first monitor\n";
    throw std::bad_cast();
  }

  const auto &spectrumInfo = WS->spectrumInfo();

  if (!spectrumInfo.hasUniqueDetector(monWI)) {
    g_log.error() << "The detector for spectrum number " << mon0Spec
                  << " was either not found or is a group, grouped monitors "
                     "are not supported by this algorithm\n";
    g_log.error() << "Error retrieving data for the first monitor\n";
    throw std::bad_cast();
  }
  monitor0Dist = spectrumInfo.l1() + spectrumInfo.l2(monWI);

  // repeat for the second detector
  try {
    monWI = WS->getIndexFromSpectrumNumber(mon1Spec);
  } catch (std::runtime_error &) {
    g_log.error()
        << "Could not find the workspace index for the monitor at spectrum "
        << mon0Spec << "\n";
    g_log.error() << "Error retrieving data for the second monitor\n";
    throw std::bad_cast();
  }
  if (!spectrumInfo.hasUniqueDetector(monWI)) {
    g_log.error() << "The detector for spectrum number " << mon1Spec
                  << " was either not found or is a group, grouped monitors "
                     "are not supported by this algorithm\n";
    g_log.error() << "Error retrieving data for the second monitor\n";
    throw std::bad_cast();
  }
  monitor1Dist = spectrumInfo.l1() + spectrumInfo.l2(monWI);
}
/** Converts detector IDs to spectra indexes
 *  @param WS :: the workspace on which the calculations are being performed
 *  @param specNum1 :: spectrum number of the output of the first monitor
 *  @param specNum2 :: spectrum number of the output of the second monitor
 *  @return the indexes of the histograms created by the detector whose ID were
 * passed
 *  @throw NotFoundError if one of the requested spectrum numbers was not found
 * in the workspace
 */
std::vector<size_t> GetEi::getMonitorWsIndexs(
    API::MatrixWorkspace_const_sptr WS, specnum_t specNum1,
    specnum_t specNum2) const { // getting spectra numbers from detector IDs is
                                // hard because the map works the other way,
  // getting index numbers from spectra numbers has
  // the same problem and we are about to do both

  // get the index number of the histogram for the first monitor

  std::vector<specnum_t> specNumTemp(&specNum1, &specNum1 + 1);
  auto wsInds = WS->getIndicesFromSpectra(specNumTemp);

  if (wsInds.size() != 1) { // the monitor spectrum isn't present in the
    // workspace, we can't continue from here
    g_log.error() << "Couldn't find the first monitor "
                     "spectrum, number "
                  << specNum1 << '\n';
    throw Exception::NotFoundError("GetEi::getMonitorWsIndexs()", specNum1);
  }

  // nowe the second monitor
  specNumTemp[0] = specNum2;
  auto wsIndexTemp = WS->getIndicesFromSpectra(specNumTemp);
  if (wsIndexTemp.size() != 1) { // the monitor spectrum isn't present in the
    // workspace, we can't continue from here
    g_log.error() << "Couldn't find the second "
                     "monitor spectrum, number "
                  << specNum2 << '\n';
    throw Exception::NotFoundError("GetEi::getMonitorWsIndexs()", specNum2);
  }

  wsInds.push_back(wsIndexTemp[0]);
  return wsInds;
}
/** Uses E_KE = mv^2/2 and s = vt to calculate the time required for a neutron
 *  to travel a distance, s
 * @param s :: ditance travelled in meters
 * @param E_KE :: kinetic energy in meV
 * @return the time to taken to travel that uninterrupted distance in seconds
 */
double GetEi::timeToFly(double s, double E_KE) const {
  // E_KE = mv^2/2, s = vt
  // t = s/v, v = sqrt(2*E_KE/m)
  // t = s/sqrt(2*E_KE/m)

  // convert E_KE to joules kg m^2 s^-2
  E_KE *= PhysicalConstants::meV;

  return s / sqrt(2 * E_KE / PhysicalConstants::NeutronMass);
}

/** Looks for and examines a peak close to that specified by the input
 * parameters and
 *  examines it to find a representative time for when the neutrons hit the
 * detector
 *  @param WS :: the workspace containing the monitor spectrum
 *  @param monitIn :: the index of the histogram that contains the monitor
 * spectrum
 *  @param peakTime :: the estimated TOF of the monitor peak in the time units
 * of the workspace
 *  @return a time of flight value in the peak in microseconds
 *  @throw invalid_argument if a good peak fit wasn't made or the input
 * workspace does not have common binning
 *  @throw out_of_range if the peak runs off the edge of the histogram
 *  @throw runtime_error a Child Algorithm just falls over
 */
double GetEi::getPeakCentre(API::MatrixWorkspace_const_sptr WS,
                            const size_t monitIn, const double peakTime) {
  const auto &timesArray = WS->x(monitIn);
  // we search for the peak only inside some window because there are often more
  // peaks in the monitor histogram
  double halfWin = (timesArray.back() - timesArray.front()) * HALF_WINDOW;
  if (monitIn < std::numeric_limits<int>::max()) {
    int ivsInd = static_cast<int>(monitIn);

    // runs CropWorkspace as a Child Algorithm to and puts the result in a new
    // temporary workspace that will be deleted when this algorithm has finished
    extractSpec(ivsInd, peakTime - halfWin, peakTime + halfWin);
  } else {
    throw Kernel::Exception::NotImplementedError(
        "Spectra number exceeds maximal"
        " integer number defined for this OS."
        " This behaviour is not yet supported");
  }
  // converting the workspace to count rate is required by the fitting algorithm
  // if the bin widths are not all the same
  WorkspaceHelpers::makeDistribution(m_tempWS);
  // look out for user cancel messgages as the above command can take a bit of
  // time
  advanceProgress(GET_COUNT_RATE);

  // to store fit results
  int64_t centreGausInd;
  double height, backGroundlev;
  getPeakEstimates(height, centreGausInd, backGroundlev);
  // look out for user cancel messgages
  advanceProgress(FIT_PEAK);

  // the peak centre is defined as the centre of the two half maximum points as
  // this is better for asymmetric peaks
  // first loop backwards along the histogram to get the first half height point
  const double lHalf =
      findHalfLoc(centreGausInd, height, backGroundlev, GO_LEFT);
  // go forewards to get the half height on the otherside of the peak
  const double rHalf =
      findHalfLoc(centreGausInd, height, backGroundlev, GO_RIGHT);
  // the peak centre is defined as the mean of the two half height times
  return (lHalf + rHalf) / 2;
}
/** Calls CropWorkspace as a Child Algorithm and passes to it the InputWorkspace
 * property
 *  @param wsInd :: the index number of the histogram to extract
 *  @param start :: the number of the first bin to include (starts counting bins
 * at 0)
 *  @param end :: the number of the last bin to include (starts counting bins at
 * 0)
 *  @throw out_of_range if start, end or wsInd are set outside of the valid
 * range for the workspace
 *  @throw runtime_error if the algorithm just falls over
 *  @throw invalid_argument if the input workspace does not have common binning
 */
void GetEi::extractSpec(int wsInd, double start, double end) {
  IAlgorithm_sptr childAlg = createChildAlgorithm(
      "CropWorkspace", 100 * m_fracCompl, 100 * (m_fracCompl + CROP));
  m_fracCompl += CROP;

  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                              getProperty("InputWorkspace"));

  childAlg->setProperty("XMin", start);
  childAlg->setProperty("XMax", end);
  childAlg->setProperty("StartWorkspaceIndex", wsInd);
  childAlg->setProperty("EndWorkspaceIndex", wsInd);
  childAlg->executeAsChildAlg();

  m_tempWS = childAlg->getProperty("OutputWorkspace");

  // DEBUGGING CODE uncomment out the line below if you want to see the TOF
  // window that was analysed
  // AnalysisDataService::Instance().addOrReplace("croped_dist_del", m_tempWS);
  progress(m_fracCompl);
  interruption_point();
}

/** Finds the largest peak by looping through the histogram and finding the
 * maximum
 *  value
 * @param height :: its passed value ignored it is set to the peak height
 * @param centreInd :: passed value is ignored it will be set to the bin index
 * of the peak center
 * @param background :: passed value ignored set mean number of counts per bin
 * in the spectrum
 * @throw invalid_argument if the peak is not clearly above the background
 */
void GetEi::getPeakEstimates(double &height, int64_t &centreInd,
                             double &background) const {

  const auto &X = m_tempWS->x(0);
  const auto &Y = m_tempWS->y(0);
  // take note of the number of background counts as error checking, do we have
  // a peak or just a bump in the background
  background = 0;
  // start at the first Y value
  height = Y[0];
  centreInd = 0;

  background = std::accumulate(Y.begin(), Y.end(), 0.0);
  // then loop through all the Y values and find the tallest peak
  // Todo use std::max to find max element and record index?
  auto maxHeight = std::max_element(Y.begin(), Y.end());
  height = *maxHeight;
  centreInd = std::distance(Y.begin(), maxHeight);

  background = background / static_cast<double>(Y.size());
  if (height < PEAK_THRESH_H * background) {
    throw std::invalid_argument(
        "No peak was found or its height is less than the threshold of " +
        boost::lexical_cast<std::string>(PEAK_THRESH_H) +
        " times the mean background, was the energy estimate (" +
        getPropertyValue("EnergyEstimate") + " meV) close enough?");
  }

  g_log.debug() << "Peak position is the bin that has the maximum Y value in "
                   "the monitor spectrum, which is at TOF "
                << (X[centreInd] + X[centreInd + 1]) / 2 << " (peak height "
                << height << " counts/microsecond)\n";
}
/** Estimates the closest time, looking either or back, when the number of
 * counts is
 *  half that in the bin whose index that passed
 *  @param startInd :: index of the bin to search around, e.g. the index of the
 * peak centre
 *  @param height :: the number of counts (or count rate) to compare against
 * e.g. a peak height
 *  @param noise :: mean number of counts in each bin in the workspace
 *  @param go :: either GetEi::GO_LEFT or GetEi::GO_RIGHT
 *  @return estimated TOF of the half maximum point
 *  @throw out_of_range if the end of the histogram is reached before the point
 * is found
 *  @throw invalid_argument if the peak is too thin
 */
double GetEi::findHalfLoc(size_t startInd, const double height,
                          const double noise, const direction go) const {
  auto endInd = startInd;

  const auto &X = m_tempWS->x(0);
  const auto &Y = m_tempWS->y(0);
  while (Y[endInd] > (height + noise) / 2.0) {
    endInd += go;
    if (endInd < 1) {
      throw std::out_of_range(
          "Can't analyse peak, some of the peak is outside the " +
          boost::lexical_cast<std::string>(HALF_WINDOW * 100) +
          "% window, at TOF values that are too low. Was the energy estimate "
          "close enough?");
    }
    if (endInd > Y.size() - 2) {
      throw std::out_of_range(
          "Can't analyse peak, some of the peak is outside the " +
          boost::lexical_cast<std::string>(HALF_WINDOW * 100) +
          "% window, at TOF values that are too high. Was the energy estimate "
          "close enough?");
    }
  }

  if (std::abs(static_cast<int64_t>(endInd - startInd)) <
      PEAK_THRESH_W) { // we didn't find a significant peak
    g_log.error() << "Likely precision problem or error, one half height "
                     "distance is less than the threshold number of bins from "
                     "the central peak: "
                  << std::abs(static_cast<int>(endInd - startInd)) << "<"
                  << PEAK_THRESH_W << ". Check the monitor peak\n";
  }
  // we have a peak in range, do an area check to see if the peak has any
  // significance
  double hOverN = (height - noise) / noise;
  if (hOverN < PEAK_THRESH_A &&
      std::abs(hOverN * static_cast<double>(endInd - startInd)) <
          PEAK_THRESH_A) { // the peak could just be noise on the background,
                           // ignore it
    throw std::invalid_argument(
        "No good peak was found. The ratio of the height to the background "
        "multiplied either half widths must be above the threshold (>" +
        boost::lexical_cast<std::string>(PEAK_THRESH_A) +
        " bins). Was the energy estimate close enough?");
  }
  // get the TOF value in the middle of the bin with the first value below the
  // half height
  double halfTime = (X[endInd] + X[endInd + 1]) / 2;
  // interpolate back between the first bin with less than half the counts to
  // the bin before it
  if (endInd != startInd) { // let the bin that we found have coordinates (x_1,
                            // y_1) the distance of the half point (x_2, y_2)
                            // from this is (y_1-y_2)/gradient. Gradient =
                            // (y_3-y_1)/(x_3-x_1) where (x_3, y_3) are the
                            // coordinates of the other bin we are using
    double gradient =
        (Y[endInd] - Y[endInd - go]) / (X[endInd] - X[endInd - go]);
    // we don't need to check for a zero or negative gradient if we assume the
    // endInd bin was found when the Y-value dropped below the threshold
    double deltaY = Y[endInd] - (height + noise) / 2.0;
    // correct for the interpolation back in the direction towards the peak
    // centre
    halfTime -= deltaY / gradient;
  }

  g_log.debug() << "One half height point found at TOF = " << halfTime
                << " microseconds\n";
  return halfTime;
}
/** Get the kinetic energy of a neuton in joules given it speed using E=mv^2/2
 *  @param speed :: the instantanious speed of a neutron in metres per second
 *  @return the energy in joules
 */
double GetEi::neutron_E_At(double speed) const {
  // E_KE = mv^2/2
  return PhysicalConstants::NeutronMass * speed * speed / (2);
}

/// Update the percentage complete estimate assuming that the algorithm has
/// completed a task with estimated RunTime toAdd
void GetEi::advanceProgress(double toAdd) {
  m_fracCompl += toAdd;
  progress(m_fracCompl);
  // look out for user cancel messgages
  interruption_point();
}

} // namespace Algorithms
} // namespace Mantid
