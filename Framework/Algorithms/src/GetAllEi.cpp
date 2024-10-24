// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GetAllEi.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <string>

namespace Mantid::Algorithms {

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
DECLARE_ALGORITHM(GetAllEi)

/// Empty default constructor
GetAllEi::GetAllEi()
    : Algorithm(), m_FilterWithDerivative(true),
      // minimal resolution for all instruments
      m_min_Eresolution(0.08),
      // half maximal resolution for LET
      m_max_Eresolution(0.5e-3), m_peakEnergyRatio2reject(0.1), m_phase(0), m_chopper(), m_pFilterLog(nullptr) {}

/// Initialization method.
void GetAllEi::init() {

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("Workspace", "", Kernel::Direction::Input),
      "The input workspace containing the monitor's spectra "
      "measured after the last chopper");
  auto nonNegative = std::make_shared<Kernel::BoundedValidator<int>>();
  nonNegative->setLower(0);

  declareProperty("Monitor1SpecID", EMPTY_INT(), nonNegative,
                  "The workspace index (ID) of the spectra, containing first monitor's"
                  " signal to analyze.");
  declareProperty("Monitor2SpecID", EMPTY_INT(), nonNegative,
                  "The workspace index (ID) of the spectra, containing second monitor's"
                  " signal to analyze.");

  declareProperty("ChopperSpeedLog", "Defined in IDF",
                  "Name of the instrument log, "
                  "containing chopper angular velocity. If 'Defined in IDF' "
                  "option is specified, "
                  "the log name is obtained from the IDF");
  declareProperty("ChopperDelayLog", "Defined in IDF",
                  "Name of the instrument log, "
                  "containing chopper delay time or chopper phase v.r.t. the pulse time. "
                  "If 'Defined in IDF'  option is specified, "
                  "the log name is obtained from IDF");
  declareProperty("FilterBaseLog", "Defined in IDF",
                  "Name of the instrument log, "
                  "with positive values indicating that instrument is running\n "
                  "and 0 or negative that it is not.\n"
                  "The log is used to identify time interval to evaluate"
                  " chopper speed and chopper delay which matter.\n"
                  "If such log is not present, average log values are calculated "
                  "within experiment start&end time range.");
  declareProperty("FilterWithDerivative", true,
                  "Use derivative of 'FilterBaseLog' "
                  "rather then log values itself to filter invalid time intervals.\n"
                  "Invalid values are then the "
                  "values where the derivative of the log turns zero.\n"
                  "E.g. the 'proton_chage' log grows for each frame "
                  "when instrument is counting and is constant otherwise.");
  setPropertySettings("FilterWithDerivative",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "FilterBaseLog", Kernel::ePropertyCriterion::IS_EQUAL_TO, "Defined in IDF"));

  auto maxInRange = std::make_shared<Kernel::BoundedValidator<double>>();
  maxInRange->setLower(1.e-6);
  maxInRange->setUpper(0.1);

  declareProperty("MaxInstrResolution", 0.0005, maxInRange,
                  "The maximal energy resolution possible for an "
                  "instrument at working energies (full width at half "
                  "maximum). \nPeaks, sharper then "
                  "this width are rejected. Accepted limits are: 1e^(-6)-0.1");

  auto minInRange = std::make_shared<Kernel::BoundedValidator<double>>();
  minInRange->setLower(0.001);
  minInRange->setUpper(0.5);
  declareProperty("MinInstrResolution", 0.08, minInRange,
                  "The minimal energy resolution possible for an "
                  "instrument at working energies (full width at half maximum).\n"
                  "Peaks broader then this width are rejected. Accepted limits are: "
                  "0.001-0.5");

  auto peakInRange = std::make_shared<Kernel::BoundedValidator<double>>();
  peakInRange->setLower(0.0);
  minInRange->setUpper(1.);
  declareProperty("PeaksRatioToReject", 0.1, peakInRange,
                  "Ratio of a peak energy to the maximal energy among all peaks. "
                  "If the ratio is lower then the value specified here, "
                  "peak is treated as insignificant and rejected.\n"
                  "Accepted limits are:0.0 (All accepted) to 1 -- only one peak \n"
                  "(or peaks with max and equal intensity) are accepted.");
  declareProperty("IgnoreSecondMonitor", false,
                  "Usually peaks are analyzed and accepted "
                  "only if identified on both monitors. If this property is set to true, "
                  "only first monitor peaks are analyzed.\n"
                  "This is debugging option as getEi has to use both monitors.");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "Name of the output matrix workspace, containing single spectra with"
      " monitor peaks energies\n"
      "together with total intensity within each peak.");
}

// unnamed namespace for auxiliary file-based compilation units
namespace {

/**Simple template function to remove invalid data from vector
 *@param guessValid -- boolean vector of indicating if each particular guess is
 *valid
 *@param guess      -- vector guess values at input and values with removing
 *                     invalid parameters at output
 */
template <class T> void removeInvalidValues(const std::vector<bool> &guessValid, std::vector<T> &guess) {
  std::vector<T> new_guess;
  new_guess.reserve(guess.size());

  for (size_t i = 0; i < guessValid.size(); i++) {
    if (guessValid[i]) {
      new_guess.emplace_back(guess[i]);
    }
  }
  new_guess.swap(guess);
}
/**Internal class to contain peak information */
struct peakKeeper {
  double position;
  double height;
  double sigma;
  double energy;

  peakKeeper(double pos, double heigh, double sig) : position(pos), height(heigh), sigma(sig) {
    this->energy = std::sqrt(2 * M_PI) * height * sigma;
  }
  // to sort peaks
  bool operator<(const peakKeeper &str) const { return (energy > str.energy); }
};

} // namespace

/** Executes the algorithm -- found all existing monitor peaks. */
void GetAllEi::exec() {
  // Get pointers to the workspace, parameter map and table
  API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");
  m_min_Eresolution = getProperty("MinInstrResolution");
  m_max_Eresolution = getProperty("MaxInstrResolution");
  m_peakEnergyRatio2reject = getProperty("PeaksRatioToReject");

  ////---> recalculate chopper delay to monitor position:
  auto pInstrument = inputWS->getInstrument();
  // auto lastChopPositionComponent =
  // pInstrument->getComponentByName("chopper-position");
  // auto chopPoint1 = pInstrument->getChopperPoint(0); ->TODO: BUG! this
  // operation loses parameters map.
  m_chopper = pInstrument->getComponentByName("chopper-position");
  if (!m_chopper)
    throw std::runtime_error("Instrument " + pInstrument->getName() + " does not have 'chopper-position' component");

  auto phase = m_chopper->getNumberParameter("initial_phase");

  if (phase.empty()) {
    throw std::runtime_error("Can not find initial_phase parameter"
                             " attached to the chopper-position component");
  }
  if (phase.size() > 1) {
    throw std::runtime_error("Can not deal with multiple phases for initial_phase"
                             " parameter attached to the chopper-position component");
  }
  m_phase = phase[0];

  this->setFilterLog(inputWS);

  // auto chopPoint1  = pInstrument->getComponentByName("fermi-chopper");
  // auto par = chopPoint1->getDoubleParameter("Delay (us)");
  double chopSpeed, chopDelay;
  findChopSpeedAndDelay(inputWS, chopSpeed, chopDelay);
  g_log.debug() << boost::str(boost::format("*Identified avrg ChopSpeed: %8.2f and Delay: %8.2f\n") % chopSpeed %
                              chopDelay);

  auto moderator = pInstrument->getSource();
  double chopDistance = m_chopper->getDistance(*moderator); // location[0].distance(moderator->getPos());
  double velocity = chopDistance / chopDelay;

  // build workspace to find monitor's peaks
  size_t det1WSIndex;
  auto monitorWS = buildWorkspaceToFit(inputWS, det1WSIndex);

  // recalculate delay time from chopper position to monitor position
  const auto &detector1 = inputWS->spectrumInfo().detector(det1WSIndex);
  double mon1Distance = detector1.getDistance(*moderator);
  double TOF0 = mon1Distance / velocity;

  //--->> below is reserved until full chopper's implementation is available;
  // auto nChoppers = pInstrument->getNumberOfChopperPoints();
  // get last chopper.
  /*
  if( nChoppers==0)throw std::runtime_error("Instrument does not have any
  choppers defined");

  auto lastChopper = pInstrument->getChopperPoint(nChoppers-1);
  ///<---------------------------------------------------
  */
  auto &baseSpectrum = inputWS->getSpectrum(det1WSIndex);
  std::pair<double, double> TOF_range = baseSpectrum.getXDataRange();

  double Period = (0.5 * 1.e+6) / chopSpeed; // 0.5 because some choppers open twice.
  // Would be nice to have it 1 or 0.5 depending on chopper type, but
  // it looks like not enough information on what chopper is available on ws;
  auto destUnit = Kernel::UnitFactory::Instance().create("Energy");

  std::vector<double> guess_opening;

  this->findGuessOpeningTimes(TOF_range, TOF0, Period, guess_opening);
  if (guess_opening.empty()) {
    throw std::runtime_error(
        "Can not find any chopper opening time within TOF range: " + boost::lexical_cast<std::string>(TOF_range.first) +
        ':' + boost::lexical_cast<std::string>(TOF_range.second));
  } else {
    destUnit->initialize(mon1Distance, static_cast<int>(Kernel::DeltaEMode::Elastic), {{Kernel::UnitParams::l2, 0.}});
    printDebugModeInfo(guess_opening, TOF_range, destUnit);
  }
  std::pair<double, double> Mon1_Erange = monitorWS->getSpectrum(0).getXDataRange();
  std::pair<double, double> Mon2_Erange = monitorWS->getSpectrum(1).getXDataRange();
  double eMin = std::max(Mon1_Erange.first, Mon2_Erange.first);
  double eMax = std::min(Mon1_Erange.second, Mon2_Erange.second);
  g_log.debug() << boost::str(boost::format("Monitors record data in energy range Emin=%8.2f; Emax=%8.2f\n") % eMin %
                              eMax);

  // convert to energy
  std::vector<double> guess_ei;
  guess_ei.reserve(guess_opening.size());
  destUnit->initialize(mon1Distance, static_cast<int>(Kernel::DeltaEMode::Elastic), {{Kernel::UnitParams::l2, 0.}});
  for (double time : guess_opening) {
    double eGuess = destUnit->singleFromTOF(time);
    if (eGuess > eMin && eGuess < eMax) {
      guess_ei.emplace_back(eGuess);
    }
  }
  g_log.debug() << "*From all chopper opening only: " + std::to_string(guess_ei.size()) +
                       " fell within both monitor's recording energy range\n";
  g_log.debug() << " Guess Energies are:\n";
  for (double ei : guess_ei) {
    g_log.debug() << boost::str(boost::format(" %8.2f; ") % ei);
  }
  g_log.debug() << '\n';

  std::sort(guess_ei.begin(), guess_ei.end());

  std::vector<size_t> irange_min, irange_max;
  std::vector<bool> guessValid;
  // preprocess first monitors peaks;
  g_log.debug() << "*Looking for real energy peaks on first monitor\n";
  findBinRanges(monitorWS->x(0), monitorWS->y(0), guess_ei, this->m_min_Eresolution / (2. * std::sqrt(2. * M_LN2)),
                irange_min, irange_max, guessValid);

  // remove invalid guess values
  removeInvalidValues<double>(guessValid, guess_ei);

  // preprocess second monitors peaks
  std::vector<size_t> irange1_min, irange1_max;
  if (!this->getProperty("IgnoreSecondMonitor")) {
    g_log.debug() << "*Looking for real energy peaks on second monitor\n";
    findBinRanges(monitorWS->x(1), monitorWS->y(1), guess_ei, this->m_min_Eresolution / (2. * std::sqrt(2. * M_LN2)),
                  irange1_min, irange1_max, guessValid);
    removeInvalidValues<double>(guessValid, guess_ei);
    removeInvalidValues<size_t>(guessValid, irange_min);
    removeInvalidValues<size_t>(guessValid, irange_max);
  } else {
    // this is wrong but will not be used anyway
    // (except formally looping through vector)
    irange1_min.assign(irange_min.begin(), irange_min.end());
    irange1_max.assign(irange_max.begin(), irange_max.end());
  }
  g_log.debug() << "*Identified: " + std::to_string(guess_ei.size()) +
                       " peaks with sufficient signal around guess chopper opening\n";

  std::vector<peakKeeper> peaks;

  double maxPeakEnergy(0);
  std::vector<size_t> monsRangeMin(2), monsRangeMax(2);
  for (size_t i = 0; i < guess_ei.size(); i++) {
    monsRangeMin[0] = irange_min[i];
    monsRangeMax[0] = irange_max[i];
    monsRangeMin[1] = irange1_min[i];
    monsRangeMax[1] = irange1_max[i];

    double energy, height, twoSigma;
    bool found = findMonitorPeak(monitorWS, guess_ei[i], monsRangeMin, monsRangeMax, energy, height, twoSigma);
    if (found) {
      peaks.emplace_back(energy, height, 0.5 * twoSigma);
      if (peaks.back().energy > maxPeakEnergy)
        maxPeakEnergy = peaks.back().energy;
    }
  }
  monitorWS.reset();

  size_t nPeaks = peaks.size();
  if (nPeaks == 0) {
    throw std::runtime_error("Can not identify any energy peaks");
  }
  // sort peaks and remove invalid one
  guessValid.resize(nPeaks);
  bool needsRemoval(false);
  for (size_t i = 0; i < nPeaks; i++) {
    peaks[i].energy /= maxPeakEnergy;
    if (peaks[i].energy < m_peakEnergyRatio2reject) {
      guessValid[i] = false;
      g_log.debug() << "*Rejecting peak at Ei=" + boost::lexical_cast<std::string>(peaks[i].position) +
                           " as its total energy lower then the threshold\n";
      needsRemoval = true;
    } else {
      guessValid[i] = true;
    }
  }
  if (needsRemoval)
    removeInvalidValues<peakKeeper>(guessValid, peaks);
  nPeaks = peaks.size();
  // sort by energy decreasing -- see class definition
  std::sort(peaks.begin(), peaks.end());

  // finalize output
  auto result_ws = create<Workspace2D>(1, Points(nPeaks));

  HistogramX peaks_positions(peaks.size());
  std::transform(peaks.cbegin(), peaks.cend(), peaks_positions.begin(), [](peakKeeper peak) { return peak.position; });
  auto &Signal = result_ws->mutableY(0);
  std::transform(peaks.cbegin(), peaks.cend(), Signal.begin(), [](peakKeeper peak) { return peak.height; });

  auto &Error = result_ws->mutableE(0);
  std::transform(peaks.cbegin(), peaks.cend(), Error.begin(), [](peakKeeper peak) { return peak.sigma; });

  result_ws->setPoints(0, peaks_positions);

  setProperty("OutputWorkspace", std::move(result_ws));
}
/**Auxiliary method to print guess chopper energies in debug mode
 *
 * @param guess_opening -- vector witgh chopper opening times values
 * @param TOF_range     -- pair describing time interval the instrument
 *                         is recording the results
 * @param destUnit      -- pointer to initialized class, converting TOF
 *                         to energy in elastic mode using instrument
 *                         parameters.
 */
void GetAllEi::printDebugModeInfo(const std::vector<double> &guess_opening, const std::pair<double, double> &TOF_range,
                                  std::shared_ptr<Kernel::Unit> &destUnit) {

  g_log.debug() << "*Found : " << guess_opening.size()
                << " chopper prospective opening within time frame: " << TOF_range.first << " to: " << TOF_range.second
                << '\n';
  g_log.debug() << " Timings are:\n";
  for (double time : guess_opening) {
    g_log.debug() << boost::str(boost::format(" %8.2f; ") % time);
  }
  g_log.debug() << '\n';
  g_log.debug() << " Corresponding to energies:\n";
  for (double time : guess_opening) {
    double ei = destUnit->singleFromTOF(time);
    g_log.debug() << boost::str(boost::format(" %8.2f; ") % ei);
  }
  g_log.debug() << '\n';
}

// unnamed namespace for auxiliary file-based functions, converted from lambda
// as not all Mantid compilers support lambda yet.

/**The internal procedure to set filter log from properties,
   defining it.
* @param inputWS -- shared pointer to the input workspace with
                    logs to analyze
*/
void GetAllEi::setFilterLog(const API::MatrixWorkspace_sptr &inputWS) {

  std::string filerLogName;
  std::string filterBase = getProperty("FilterBaseLog");
  if (boost::iequals(filterBase, "Defined in IDF")) {
    filerLogName = m_chopper->getStringParameter("FilterBaseLog")[0];
    m_FilterWithDerivative = m_chopper->getBoolParameter("filter_with_derivative")[0];
  } else {
    filerLogName = filterBase;
    m_FilterWithDerivative = getProperty("FilterWithDerivative");
  }
  try {
    m_pFilterLog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(inputWS->run().getProperty(filerLogName));
  } catch (std::runtime_error &) {
    g_log.warning() << " Can not retrieve (double) filtering log: " + filerLogName +
                           " from current workspace\n"
                           " Using total experiment range to "
                           "find logs averages for chopper parameters\n";
    m_FilterWithDerivative = false;
  }
}
/**Former lambda to identify guess values for a peak at given index
 * and set up these parameters as input for fitting algorithm
 *
 *@param inputWS -- the workspace to process
 *@param index -- the number of the workspace spectra to process
 *@param Ei           -- incident energy
 *@param monsRangeMin -- vector of left boundaries for the subintervals to look
 *for peak
 *@param monsRangeMax -- vector of right boundaries for the subintervals to look
 *for peak
 *
 *@param peakPos      -- output energy of the peak
 *@param peakHeight   -- output height of the peak assuming Gaussian shape
 *@param peakTwoSigma -- output width of the peak assuming Gaussian shape
 */
bool GetAllEi::peakGuess(const API::MatrixWorkspace_sptr &inputWS, size_t index, double Ei,
                         const std::vector<size_t> &monsRangeMin, const std::vector<size_t> &monsRangeMax,
                         double &peakPos, double &peakHeight, double &peakTwoSigma) {

  // calculate sigma from half-width parameters
  double maxSigma = Ei * m_min_Eresolution / (2. * std::sqrt(2. * M_LN2));

  double sMin(std::numeric_limits<double>::max());
  double sMax(-sMin);
  double xOfMax(0), dXmax(0);
  double Intensity(0);

  const auto &X = inputWS->x(index);
  const auto &S = inputWS->y(index);
  size_t ind_min = monsRangeMin[index];
  size_t ind_max = monsRangeMax[index];
  // interval too small -- not interested in a peak there
  if (std::fabs(double(ind_max - ind_min)) < 5)
    return false;

  // double xMin = X[ind_min];
  // double xMax = X[ind_max];
  // size_t ind_Ofmax(ind_min);

  for (size_t i = ind_min; i < ind_max; i++) {
    double dX = X[i + 1] - X[i];
    double signal = S[i] / dX;
    if (signal < sMin)
      sMin = signal;
    if (signal > sMax) {
      sMax = signal;
      dXmax = dX;
      xOfMax = X[i];
      // ind_Ofmax=i;
    }
    Intensity += S[i];
  }
  // monitor peak should not have just two counts in it.
  if (sMax * dXmax <= 2)
    return false;
  //
  // size_t SearchAreaSize = ind_max - ind_min;

  double SmoothRange = 2 * maxSigma;

  std::vector<double> SAvrg, binsAvrg;
  Kernel::VectorHelper::smoothInRange(S.rawData(), SAvrg, SmoothRange, &X.rawData(), ind_min, ind_max, &binsAvrg);

  double realPeakPos(xOfMax); // this position is less shifted
  // due to the skew in averaging formula
  bool foundRealPeakPos(false);
  std::vector<double> der1Avrg, der2Avrg, peaks, hillsPos, SAvrg1, binsAvrg1;
  size_t nPeaks = this->calcDerivativeAndCountZeros(binsAvrg, SAvrg, der1Avrg, peaks);
  size_t nHills = this->calcDerivativeAndCountZeros(binsAvrg, der1Avrg, der2Avrg, hillsPos);
  if (nPeaks == 1) {
    foundRealPeakPos = true;
    realPeakPos = peaks[0];
  }

  size_t ic(0), stay_still_count(0);
  bool iterations_fail(false);
  while ((nPeaks > 1 || nHills > 2) && (!iterations_fail)) {
    Kernel::VectorHelper::smoothInRange(SAvrg, SAvrg1, SmoothRange, &binsAvrg, 0, ind_max - ind_min, &binsAvrg1);
    const auto nPrevHills = nHills;

    nPeaks = this->calcDerivativeAndCountZeros(binsAvrg1, SAvrg1, der1Avrg, peaks);
    nHills = this->calcDerivativeAndCountZeros(binsAvrg1, der1Avrg, der2Avrg, hillsPos);
    SAvrg.swap(SAvrg1);
    binsAvrg.swap(binsAvrg1);
    if (nPeaks == 1 && !foundRealPeakPos) { // fix first peak position found
      foundRealPeakPos = true;              // as averaging shift peaks on
      realPeakPos = peaks[0];               // irregular grid.
    }
    ic++;
    if (nPrevHills <= nHills) {
      stay_still_count++;
    } else {
      stay_still_count = 0;
    }
    if (ic > 50 || stay_still_count > 3)
      iterations_fail = true;
  }
  if (iterations_fail) {
    g_log.information() << "*No peak search convergence after " + std::to_string(ic) +
                               " smoothing iterations at still_count: " + std::to_string(stay_still_count) +
                               " Wrong energy or noisy peak at Ei=" + boost::lexical_cast<std::string>(Ei)
                        << '\n';
  }
  g_log.debug() << "*Performed: " + std::to_string(ic) + " averages for spectra " + std::to_string(index) +
                       " at energy: " + boost::lexical_cast<std::string>(Ei) +
                       "\n and found: " + std::to_string(nPeaks) + "peaks and " + std::to_string(nHills) + " hills\n";
  if (nPeaks != 1) {
    g_log.debug() << "*Peak rejected as n-peaks !=1 after averaging\n";
    return false;
  }

  peakPos = peaks[0];
  if (nHills > 2) {
    auto peakIndex = std::size_t(Kernel::VectorHelper::getBinIndex(hillsPos, peaks[0]));
    peakTwoSigma = hillsPos[peakIndex + 1] - hillsPos[peakIndex];
  } else {
    if (hillsPos.size() == 2) {
      peakTwoSigma = hillsPos[1] - hillsPos[0];
    } else {
      g_log.debug() << "*Peak rejected as averaging gives: " + std::to_string(nPeaks) + " peaks and " +
                           std::to_string(nHills) + " heals\n";

      return false;
    }
  }
  // assuming that averaging conserves intensity and removing linear
  // background:
  peakHeight = Intensity / (0.5 * std::sqrt(2. * M_PI) * peakTwoSigma) - sMin;
  peakPos = realPeakPos;

  return true;
}

/**Get energy of monitor peak if one is present
 *@param inputWS -- the workspace to process
 *@param Ei           -- incident energy
 *@param monsRangeMin -- vector of indexes of left boundaries
 *                       for the subintervals to look for peak
 *@param monsRangeMax -- vector of indexes of right boundaries
 *                       for the subintervals to look for peak
 *
 *@param position     -- output energy of the peak center.
 *@param height       -- output height of the peak assuming Gaussian shape
 *@param twoSigma     -- output width of the peak assuming Gaussian shape
 */
bool GetAllEi::findMonitorPeak(const API::MatrixWorkspace_sptr &inputWS, double Ei,
                               const std::vector<size_t> &monsRangeMin, const std::vector<size_t> &monsRangeMax,
                               double &position, double &height, double &twoSigma) {
  // calculate sigma from half-width parameters
  double maxSigma = Ei * m_min_Eresolution / (2. * std::sqrt(2. * M_LN2));
  double minSigma = Ei * m_max_Eresolution / (2. * std::sqrt(2. * M_LN2));
  //--------------------------------------------------------------------
  double peak1Pos, peak1TwoSigma, peak1Height;
  if (!peakGuess(inputWS, 0, Ei, monsRangeMin, monsRangeMax, peak1Pos, peak1Height, peak1TwoSigma))
    return false;
  if (0.25 * peak1TwoSigma > maxSigma || peak1TwoSigma < minSigma) {
    g_log.debug() << "*Rejecting due to width: Peak at mon1 Ei=" + boost::lexical_cast<std::string>(peak1Pos) +
                         " with Height:" + boost::lexical_cast<std::string>(peak1Height) +
                         " and 2*Sigma: " + boost::lexical_cast<std::string>(peak1TwoSigma)
                  << '\n';
    return false;
  }

  if (!this->getProperty("IgnoreSecondMonitor")) {
    double peak2Pos, peak2TwoSigma, peak2Height;
    if (!peakGuess(inputWS, 1, Ei, monsRangeMin, monsRangeMax, peak2Pos, peak2Height, peak2TwoSigma))
      return false;
    // Let's not check anything except peak position for monitor2, as
    // its intensity may be very low for some instruments.
    // if(0.25*peak2TwoSigma>maxSigma||peak2TwoSigma<minSigma)return false;

    // peak in first and second monitors are too far from each other. May be the
    // instrument
    // is ill-calibrated but GetEi will probably not find this peak anyway.
    if (std::fabs(peak1Pos - peak2Pos) > 0.25 * (peak1TwoSigma + peak2TwoSigma)) {
      g_log.debug() << "*Rejecting due to displacement between Peak at mon1: Ei=" +
                           boost::lexical_cast<std::string>(peak1Pos) +
                           " with Height:" + boost::lexical_cast<std::string>(peak1Height) +
                           " and 2*Sigma: " + boost::lexical_cast<std::string>(peak1TwoSigma) +
                           "\n and Peak at mon2: Ei= " + boost::lexical_cast<std::string>(peak2Pos) +
                           "and height: " + boost::lexical_cast<std::string>(peak1Height)
                    << '\n';

      return false;
    }
  }

  position = peak1Pos;
  twoSigma = peak1TwoSigma;
  height = peak1Height;

  return true;
}
namespace { // for lambda extracted from calcDerivativeAndCountZeros
            /**former lambda from calcDerivativeAndCountZeros
             *estimating if sign have changed from its previous value
             *@param val -- current function value
             *@param prevSign -- the sign of the function at previous value
             */
bool signChanged(double val, int &prevSign) {
  int curSign = (val >= 0 ? 1 : -1);
  bool changed = curSign != prevSign;
  prevSign = curSign;
  return changed;
}
} // namespace

/**Bare-bone function to calculate numerical derivative, and estimate number of
* zeros this derivative has. The function is assumed to be defined on the
* the left of a bin range so the derivative is calculated in the same point.
* No checks are performed for simplicity so data have to be correct
* form at input.
*@param bins -- vector of bin boundaries.
*@param signal  -- vector of signal size of bins.size()-1
*@param deriv   -- output vector of numerical derivative
*@param zeros   -- coordinates of found zeros

*@return -- number of zeros, the derivative has in the interval provided.
*/
size_t GetAllEi::calcDerivativeAndCountZeros(const std::vector<double> &bins, const std::vector<double> &signal,
                                             std::vector<double> &deriv, std::vector<double> &zeros) {
  size_t nPoints = signal.size();
  deriv.resize(nPoints);
  zeros.resize(0);

  std::list<double> funVal;
  double bin0 = bins[1] - bins[0];
  double f0 = signal[0] / bin0;
  double bin1 = bins[2] - bins[1];
  double f1 = signal[1] / bin1;

  size_t nZeros(0);

  funVal.push_front(f1);
  deriv[0] = 2 * (f1 - f0) / (bin0 + bin1);
  int prevSign = (deriv[0] >= 0 ? 1 : -1);

  for (size_t i = 1; i < nPoints - 1; i++) {
    bin1 = (bins[i + 2] - bins[i + 1]);
    f1 = signal[i + 1] / bin1;
    deriv[i] = (f1 - f0) / (bins[i + 2] - bins[i]);
    f0 = funVal.back();
    funVal.pop_back();
    funVal.push_front(f1);

    if (signChanged(deriv[i], prevSign)) {
      nZeros++;
      zeros.emplace_back(0.5 * (bins[i - 1] + bins[i]));
    }
  }
  deriv[nPoints - 1] = 2 * (f1 - f0) / (bin1 + bin0);
  if (signChanged(deriv[nPoints - 1], prevSign)) {
    zeros.emplace_back(bins[nPoints - 1]);
    nZeros++;
  }

  return nZeros;
}
namespace { // for lambda extracted from findBinRanges
// get bin range corresponding to the energy range
void getBinRange(const HistogramData::HistogramX &eBins, double eMin, double eMax, size_t &index_min,
                 size_t &index_max) {

  const auto &bins = eBins.rawData();
  const size_t nBins = bins.size();
  if (eMin <= bins[0]) {
    index_min = 0;
  } else {
    index_min = std::size_t(Kernel::VectorHelper::getBinIndex(bins, eMin));
  }

  if (eMax >= eBins[nBins - 1]) {
    index_max = nBins - 1;
  } else {
    index_max = std::size_t(Kernel::VectorHelper::getBinIndex(bins, eMax)) + 1;
    if (index_max >= nBins)
      index_max = nBins - 1; // last bin range anyway. Should not happen
  }
}

// refine bin range. May need better procedure for this.
bool refineEGuess(const HistogramX &eBins, const HistogramY &signal, double &eGuess, size_t index_min,
                  size_t index_max) {

  size_t ind_Emax = index_min;
  double SMax(0);
  for (size_t i = index_min; i < index_max; i++) {
    double dX = eBins[i + 1] - eBins[i];
    double sig = signal[i] / dX;
    if (sig > SMax) {
      SMax = sig;
      ind_Emax = i;
    }
  }
  if (ind_Emax == index_min || ind_Emax == index_max) {
    return false;
  }
  eGuess = 0.5 * (eBins[ind_Emax] + eBins[ind_Emax + 1]);
  return true;
}

struct peakKeeper2 {
  double left_rng;
  double right_rng;
  peakKeeper2() : left_rng(.0), right_rng(.0) {};
  peakKeeper2(double left, double right) : left_rng(left), right_rng(right) {}
};
} // namespace

/**Find indexes of each expected peak intervals from monotonous array of ranges.
 *@param eBins   -- bin ranges to look through
 *@param signal  -- vector of signal in the bins
 *@param guess_energy -- vector of guess energies to look for
 *@param  eResolution -- instrument resolution in energy units
 *@param irangeMin  -- start indexes of energy intervals in the guess_energies
 *                     vector.
 *@param irangeMax  -- final indexes of energy intervals in the guess_energies
 *                     vector.
 *@param guessValid -- output boolean vector, which specifies if guess energies
 *                     in guess_energy vector are valid or not
 */
void GetAllEi::findBinRanges(const HistogramX &eBins, const HistogramY &signal, const std::vector<double> &guess_energy,
                             double eResolution, std::vector<size_t> &irangeMin, std::vector<size_t> &irangeMax,
                             std::vector<bool> &guessValid) {

  // size_t nBins = eBins.size();
  guessValid.resize(guess_energy.size());

  // Do the job
  size_t ind_min, ind_max;
  irangeMin.resize(0);
  irangeMax.resize(0);

  // identify guess bin ranges
  std::vector<peakKeeper2> guess_peak(guess_energy.size());
  for (size_t nGuess = 0; nGuess < guess_energy.size(); nGuess++) {
    double eGuess = guess_energy[nGuess];
    getBinRange(eBins, eGuess * (1 - 4 * eResolution), eGuess * (1 + 4 * eResolution), ind_min, ind_max);
    guess_peak[nGuess] = peakKeeper2(eBins[ind_min], eBins[ind_max]);
  }
  // verify that the ranges not intercept and refine interceptions
  for (size_t i = 1; i < guess_energy.size(); i++) {
    if (guess_peak[i - 1].right_rng > guess_peak[i].left_rng) {
      double mid_pnt = 0.5 * (guess_peak[i - 1].right_rng + guess_peak[i].left_rng);
      guess_peak[i - 1].right_rng = mid_pnt;
      guess_peak[i].left_rng = mid_pnt;
    }
  }
  // identify final bin ranges
  for (size_t nGuess = 0; nGuess < guess_energy.size(); nGuess++) {

    double eGuess = guess_energy[nGuess];
    getBinRange(eBins, guess_peak[nGuess].left_rng, guess_peak[nGuess].right_rng, ind_min, ind_max);

    if (refineEGuess(eBins, signal, eGuess, ind_min, ind_max)) {
      getBinRange(eBins, std::max(guess_peak[nGuess].left_rng, eGuess * (1 - 3 * eResolution)),
                  std::max(guess_peak[nGuess].right_rng, eGuess * (1 + 3 * eResolution)), ind_min, ind_max);
      irangeMin.emplace_back(ind_min);
      irangeMax.emplace_back(ind_max);
      guessValid[nGuess] = true;
    } else {
      guessValid[nGuess] = false;
      g_log.debug() << "*Incorrect guess energy: " << boost::lexical_cast<std::string>(eGuess)
                    << " no energy peak found in 4*Sigma range\n";
    }
  }
  // if array decreasing rather then increasing, indexes behave differently.
  // Will it still work?
  if (!irangeMax.empty()) {
    if (irangeMax[0] < irangeMin[0]) {
      irangeMax.swap(irangeMin);
    }
  }
}

/**Build 2-spectra workspace in units of energy, used as source
 *to identify actual monitors spectra
 *@param inputWS shared pointer to initial workspace
 *@param wsIndex0 -- returns workspace index for first detector.
 *@return shared pointer to intermediate workspace, in units of energy
 *        used to fit monitor's spectra.
 */
API::MatrixWorkspace_sptr GetAllEi::buildWorkspaceToFit(const API::MatrixWorkspace_sptr &inputWS, size_t &wsIndex0) {

  // at this stage all properties are validated so its safe to access them
  // without
  // additional checks.
  specnum_t specNum1 = getProperty("Monitor1SpecID");
  wsIndex0 = inputWS->getIndexFromSpectrumNumber(specNum1);
  specnum_t specNum2 = getProperty("Monitor2SpecID");
  size_t wsIndex1 = inputWS->getIndexFromSpectrumNumber(specNum2);

  // assuming equally binned ws.
  std::shared_ptr<API::HistoWorkspace> working_ws = DataObjects::create<API::HistoWorkspace>(
      *inputWS, Indexing::extract(inputWS->indexInfo(), std::vector<size_t>{wsIndex0, wsIndex1}),
      inputWS->histogram(wsIndex0));

  // signal 1
  working_ws->setSharedY(0, inputWS->sharedY(wsIndex0));
  // signal 2
  working_ws->setSharedY(1, inputWS->sharedY(wsIndex1));
  // error 1
  working_ws->setSharedE(0, inputWS->sharedE(wsIndex0));
  // error 2
  working_ws->setSharedE(1, inputWS->sharedE(wsIndex1));

  if (inputWS->getAxis(0)->unit()->caption() != "Energy") {
    auto conv = createChildAlgorithm("ConvertUnits");
    conv->initialize();
    conv->setProperty("InputWorkspace", working_ws);
    conv->setProperty("OutputWorkspace", working_ws);
    conv->setPropertyValue("Target", "Energy");
    conv->setPropertyValue("EMode", "Elastic");
    // conv->setProperty("AlignBins",true); --> throws due to bug in
    // ConvertUnits
    conv->execute();
  }

  return working_ws;
}
/**function calculates list of provisional chopper opening times.
*@param TOF_range -- std::pair containing min and max time, of signal
*                    measured on monitors
*@param  ChopDelay -- the time of flight neutrons travel from source
*                     to the chopper opening.
*@param  Period  -- period of chopper openings

*@param guess_opening_times -- output vector with time values
*                         at which neutrons may pass through the chopper.
*/
void GetAllEi::findGuessOpeningTimes(const std::pair<double, double> &TOF_range, double ChopDelay, double Period,
                                     std::vector<double> &guess_opening_times) {

  if (ChopDelay >= TOF_range.second) {
    std::string chop = boost::str(boost::format("%.2g") % ChopDelay);
    std::string t_min = boost::str(boost::format("%.2g") % TOF_range.first);
    std::string t_max = boost::str(boost::format("%.2g") % TOF_range.second);
    throw std::runtime_error("Logical error: Chopper opening time: " + chop + " is outside of time interval: " + t_min +
                             ":" + t_max + " of the signal, measured on monitors.");
  }

  // number of times chopper with specified rotation period opens.
  size_t n_openings = static_cast<size_t>((TOF_range.second - ChopDelay) / Period) + 1;
  // number of periods falling outside of the time period, measuring on monitor.
  size_t n_start(0);
  if (ChopDelay < TOF_range.first) {
    n_start = static_cast<size_t>((TOF_range.first - ChopDelay) / Period) + 1;
    n_openings -= n_start;
  }

  guess_opening_times.resize(n_openings);
  for (size_t i = n_start; i < n_openings + n_start; i++) {
    guess_opening_times[i - n_start] = ChopDelay + static_cast<double>(i) * Period;
  }
}
/**Finds pointer to log value for the property with the name provided
 *
 *@param inputWS      -- workspace with logs attached
 *@param propertyName -- name of the property to find log for
 *
 *@return -- pointer to property which contain the log requested or nullptr if
 *           no log found or other errors identified.  */
Kernel::Property *GetAllEi::getPLogForProperty(const API::MatrixWorkspace_sptr &inputWS,
                                               const std::string &propertyName) {

  std::string LogName = this->getProperty(propertyName);
  if (boost::iequals(LogName, "Defined in IDF")) {
    auto AllNames = m_chopper->getStringParameter(propertyName);
    if (AllNames.size() != 1)
      return nullptr;
    LogName = AllNames[0];
  }
  auto pIProperty = (inputWS->run().getProperty(LogName));

  return pIProperty;
}

/**Return average time series log value for the appropriately filtered log
 * @param inputWS      -- shared pointer to the input workspace containing
 *                        the log to process
 * @param propertyName -- log name
 * @param timeroi      -- used to filter input events or empty to use
 *                        experiment start/end times.
 */
double GetAllEi::getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS, const std::string &propertyName,
                                 Kernel::TimeROI &timeroi) {

  auto pIProperty = getPLogForProperty(inputWS, propertyName);

  // this will always provide a defined pointer as this has been verified in
  // validator.
  auto pTimeSeries = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pIProperty);

  if (!pTimeSeries) {
    throw std::runtime_error("Could not retrieve a time series property for the property name " + propertyName);
  }

  double value;
  if (timeroi.useAll()) {
    const auto timeStart = inputWS->run().startTime();
    const auto timeStop = inputWS->run().endTime();
    if (timeStart < timeStop) {
      Kernel::TimeROI localROI(timeStart, timeStop);
      value = pTimeSeries->getStatistics(&localROI).mean;
    } else {
      // all values in pTimeSeries will be used
      value = pTimeSeries->getStatistics().mean;
    }
  } else {
    value = pTimeSeries->getStatistics(&timeroi).mean;
  }
  // statistics returns nan as mean of empty vector
  if (std::isnan(value)) {
    throw std::runtime_error("Can not find average value for log defined by property" + propertyName +
                             " As no valid log values are found.");
  }

  return value;
}
namespace { // former lambda function for findChopSpeedAndDelay

/**Select time interval on the basis of previous time interval
 * selection and check if current value gets in the selection
 *
 * @param t_beg -- initial time for current time interval
 * @param t_end -- final time for current time interval
 * @param inSelection -- the boolean indicating if previous interval
 *                       was selected on input and current selected on
 *                       output
 * @param startTime -- total selection time start moment
 * @param endTime   -- total selection time final moments
 *
 *@return true if selection interval is completed
 *        (current interval is not selected) and false otherwise
 */
bool SelectInterval(const Types::Core::DateAndTime &t_beg, const Types::Core::DateAndTime &t_end, double value,
                    bool &inSelection, Types::Core::DateAndTime &startTime, Types::Core::DateAndTime &endTime) {

  if (value > 0) {
    if (!inSelection) {
      startTime = t_beg;
    }
    inSelection = true;
  } else {
    if (inSelection) {
      inSelection = false;
      if (endTime > startTime)
        return true;
    }
  }
  endTime = t_end;
  return false;
}
} // namespace
/**Analyze chopper logs and identify chopper speed and delay
@param  inputWS    -- sp to workspace with attached logs.
@param chop_speed -- output value for chopper speed in uSec
@param chop_delay -- output value for chopper delay in uSec
*/
void GetAllEi::findChopSpeedAndDelay(const API::MatrixWorkspace_sptr &inputWS, double &chop_speed, double &chop_delay) {

  // TODO: Make it dependent on inputWS time range

  Kernel::TimeROI timeroi;
  if (m_pFilterLog) {
    std::unique_ptr<Kernel::TimeSeriesProperty<double>> pDerivative;

    // Define selecting function
    bool inSelection(false);
    // time interval to select (start-end)
    Types::Core::DateAndTime startTime, endTime;
    //
    // Analyze filtering log
    auto dateAndTimes = m_pFilterLog->valueAsCorrectMap();
    auto it = dateAndTimes.begin();
    auto next = it;
    next++;
    std::map<Types::Core::DateAndTime, double> derivMap;
    auto itder = it;
    if (m_FilterWithDerivative) {
      pDerivative = m_pFilterLog->getDerivative();
      derivMap = pDerivative->valueAsCorrectMap();
      itder = derivMap.begin();
    }

    // initialize selection log
    if (dateAndTimes.size() <= 1) {
      SelectInterval(it->first, it->first, itder->second, inSelection, startTime, endTime);
      if (inSelection) {
        startTime = inputWS->run().startTime();
        endTime = inputWS->run().endTime();
        timeroi.addROI(startTime, endTime);
      } else {
        throw std::runtime_error("filtered all data points. Nothing to do");
      }
    } else {
      SelectInterval(it->first, next->first, itder->second, inSelection, startTime, endTime);
    }

    // if its filtered using log, both iterator walk through the same values
    // if use derivative, derivative's values are used for filtering
    // and derivative assumed in a center of an interval
    for (; next != dateAndTimes.end(); ++next, ++itder) {
      if (SelectInterval(it->first, next->first, itder->second, inSelection, startTime, endTime)) {
        timeroi.addROI(startTime, endTime);
      }
      it = next;
    }
    // final interval
    if (inSelection && (endTime > startTime)) {
      timeroi.addROI(startTime, endTime);
    }
  } // End of USE filter log.

  chop_speed = this->getAvrgLogValue(inputWS, "ChopperSpeedLog", timeroi);
  chop_speed = std::fabs(chop_speed);
  if (chop_speed < 1.e-7) {
    throw std::runtime_error("Chopper speed can not be zero ");
  }
  chop_delay = std::fabs(this->getAvrgLogValue(inputWS, "ChopperDelayLog", timeroi));

  // process chopper delay in the units of degree (phase)
  auto pProperty = getPLogForProperty(inputWS, "ChopperDelayLog");
  if (!pProperty)
    throw std::runtime_error("ChopperDelayLog has been removed from workspace "
                             "during the algorithm execution");
  std::string units = pProperty->units();
  // its chopper phase provided
  if (units == "deg" || units.c_str()[0] == -80) { //<- userd in ISIS ASCII representation of o(deg)
    chop_delay *= 1.e+6 / (360. * chop_speed);     // convert in uSec
  }
  chop_delay += m_phase / chop_speed;
}

namespace { // namespace for lambda functions, used in validators

/* former Lambda to validate if appropriate log is present in workspace
and if it's present, it is a time-series property
* @param prop_name    -- the name of the log to check
* @param err_presence -- core error message to return if no log found
* @param err_type     -- core error message to return if
*                        log is of incorrect type
* @param fail         -- fail or warn if appropriate log is not available.
*
* @param result       -- map to add the result of check for errors
*                       if no error found the map is not modified and remains
*                       empty.


* @return             -- false if all checks are fine, or true if check is
*                        failed
*/
bool check_time_series_property(const GetAllEi *algo, const API::MatrixWorkspace_sptr &inputWS,
                                const std::shared_ptr<const Geometry::IComponent> &chopper,
                                const std::string &prop_name, const std::string &err_presence,
                                const std::string &err_type, bool fail, std::map<std::string, std::string> &result) {

  std::string LogName = algo->getProperty(prop_name);
  if (boost::iequals(LogName, "Defined in IDF")) {
    try {
      auto theLogs = chopper->getStringParameter(prop_name);
      if (theLogs.empty()) {
        if (fail)
          result[prop_name] = "Can not retrieve parameter " + prop_name + " from the instrument definition file.";
        return true;
      }
      LogName = theLogs[0];
    } catch (...) {
      result[prop_name] = "Can not retrieve parameter " + prop_name + " from the instrument definition file.";
      return true;
    }
  }
  try {
    Kernel::Property *pProp = inputWS->run().getProperty(LogName);
    auto pTSProp = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProp);
    if (!pTSProp) {
      if (fail)
        result[prop_name] = "Workspace contains " + err_type + LogName + " But its type is not a timeSeries property";
      return true;
    }
  } catch (std::runtime_error &) {
    if (fail)
      result[prop_name] = "Workspace has to contain " + err_presence + LogName;
    return true;
  }
  return false;
}
} // namespace

/**Validates if input workspace contains all necessary logs and if all
*  these logs are the logs of appropriate type
@return list of invalid logs or empty list if no errors is found.
*/
std::map<std::string, std::string> GetAllEi::validateInputs() {

  // Do Validation
  std::map<std::string, std::string> result;

  API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");
  if (!inputWS) {
    result["Workspace"] = "Input workspace can not be identified";
    return result;
  }
  if (!inputWS->isHistogramData()) {
    result["Workspace"] = "Only histogram workspaces are currently supported. "
                          "Rebin input workspace first.";
  }

  specnum_t specNum1 = getProperty("Monitor1SpecID");
  try {
    inputWS->getIndexFromSpectrumNumber(specNum1);
  } catch (std::runtime_error &) {
    result["Monitor1SpecID"] = "Input workspace does not contain spectra with ID: " + std::to_string(specNum1);
  }
  specnum_t specNum2 = getProperty("Monitor2SpecID");
  try {
    inputWS->getIndexFromSpectrumNumber(specNum2);
  } catch (std::runtime_error &) {
    result["Monitor2SpecID"] = "Input workspace does not contain spectra with ID: " + std::to_string(specNum2);
  }
  // check chopper and initiate it if present (for debugging)
  m_chopper = inputWS->getInstrument()->getComponentByName("chopper-position");
  if (!m_chopper) {
    result["Workspace_chopper"] = " For this algorithm to work workspace has"
                                  " to contain well defined 'chopper-position' component";
    return result;
  }

  check_time_series_property(this, inputWS, m_chopper, "ChopperSpeedLog",
                             "chopper speed log with name: ", "chopper speed log ", true, result);
  check_time_series_property(this, inputWS, m_chopper, "ChopperDelayLog",
                             "property related to chopper delay log with name: ", "chopper delay log ", true, result);
  bool failed = check_time_series_property(this, inputWS, m_chopper, "FilterBaseLog",
                                           "filter base log named: ", "filter base log: ", false, result);
  if (failed) {
    g_log.warning() << " Can not find a log to identify good DAE operations.\n"
                       " Assuming that good operations start from experiment time=0";
  } else {
    this->setFilterLog(inputWS);
  }
  return result;
}

} // namespace Mantid::Algorithms
