#include "MantidAlgorithms/EQSANSUnfoldFrame.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/EQSANSUnfoldFrame.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <vector>

#include <vector>

namespace Mantid {
namespace Algorithms {

// #########################################
// ###############   EQSANSWBand   ###############
// #########################################

/// Default constructor
EQSANSWBand::EQSANSWBand() : m_min(0.0), m_max(0.0) {}

/**
 * Constructor with arguments
 * @param lMin Lower boundary wavelength
 * @param lMax Upper boundary wavelength
 * @throw std::range_error when lMin > lMax. Null bands (lMin==lMax) are
 * permitted
 * @throw std::value_error for negative lMin or lMax
 */
EQSANSWBand::EQSANSWBand(const double &lMin, const double &lMax)
    : m_min(lMin), m_max(lMax) {
  if (lMin < 0.0 || lMax < 0.0)
    throw std::domain_error("Negative wavelengths are unphysical");
  if (lMin > lMax)
    throw std::range_error("Negative bandwidths are unphysical");
}

double EQSANSWBand::width() { return m_max - m_min; }

/**
 * Find the common band between two bands.
 * @param band
 * @returns command band. Returns WBand(0.0, 0.0) if no common band or
 * if commom band is only one point, like (0.0, 1.0) and (1.0, 2.0)
 */
EQSANSWBand EQSANSWBand::intersect(const EQSANSWBand &band) const {
  double a = m_min > band.m_min ? m_min : band.m_min;
  double b = m_max < band.m_max ? m_max : band.m_max;
  if (a >= b) {
    a = 0.0;
    b = 0.0;
  }
  return EQSANSWBand(a, b);
}

// ###############################################
// ###############   EQSANSTransWBands   ###############
// ###############################################

EQSANSTransWBands::EQSANSTransWBands() : m_bands(0) {}

/// Number of wavelengths bands
size_t EQSANSTransWBands::size() { return m_bands.size(); }

/// common band(s) between the transmissions bands and one external band
EQSANSTransWBands
EQSANSTransWBands::intersect(const EQSANSWBand &otherBand) const {
  EQSANSTransWBands wg;
  for (const EQSANSWBand &band : m_bands) {
    auto b = band.intersect(otherBand);
    if (b.width() > 0.0)
      wg.m_bands.emplace_back(b);
  }
  return wg;
}

/**
 * Find the common wavelength bands between two transmission bands set
 * @param gates transmission bands set acting as wavelength filtering gates
 */
EQSANSTransWBands
EQSANSTransWBands::intersect(const EQSANSTransWBands &gates) const {
  EQSANSTransWBands wg;
  for (const EQSANSWBand &gate : gates.m_bands) {
    auto g = this->intersect(gate);
    if (g.size() > 0)
      wg.m_bands.insert(std::end(wg.m_bands), std::begin(g.m_bands),
                        std::end(g.m_bands));
  }
  return wg;
}

// #####################################################
// ###############   EQSANSDiskChopper   ###############
// #####################################################

EQSANSDiskChopper::EQSANSDiskChopper()
    : m_index(0), m_location(0.0), m_aperture(0.0), m_phase(0.0), m_speed(0.0) {
}

/// Time required by the chopper for a full spin, in microseconds
double EQSANSDiskChopper::period() const { return 1.0e6 / m_speed; }

/// Time for the transmission window to spin the whole aperture, in microseconds
double EQSANSDiskChopper::transmissionDuration() const {
  return period() * (m_aperture / 360.0);
}

/// Time required by the opening edge of the transmission to hit the beam, in
/// microseconds
double EQSANSDiskChopper::openingPhase() const {
  return m_phase - 0.5 * transmissionDuration();
}

/// Time required by the closing edge of the transmission to hit the beam, in
/// microseconds
double EQSANSDiskChopper::closingPhase() const {
  return m_phase + 0.5 * transmissionDuration();
}

/**
 * Rewind the chopper to obtain the minimum positive time for the closing
 * edge of the transmission window to hit axis defined by the neutron beam
 * @return phase of the opening edge when the chopper is rewound. Can return
 * negative valu
 */
double EQSANSDiskChopper::rewind() const {
  double t_opening;
  double t_closing = closingPhase();
  if (t_closing < 0.0) {
    while (t_closing < 0.0)
      t_closing += period();
    t_opening = t_closing - transmissionDuration();
  } else {
    while (t_closing > 0.0)
      t_closing -= period();
    t_opening = t_closing + period() - transmissionDuration();
  }
  return t_opening;
}

/**
 * Convert time of flight to neutron's wavelength.
 * @param tof time of flight, in microseconds
 * @param delay Additional time-of-flight to include in the calculations
 * @param pulsed Include the correction due to delayed emission of neutrons from
 * the moderator
 * @return neutron wavelength (in Angstroms). Returns zero for negative input
 * tof.
 */
double EQSANSDiskChopper::tofToWavelength(double tof, double delay,
                                          bool pulsed) const {
  double sigma = 3.9560346e-03; // plank constant divided by neutron mass
  double effective_location =
      pulsed ? (m_location + sigma * PULSEWIDTH) : m_location;
  double wl = sigma * (tof + delay) / effective_location;
  if (wl < 0.0)
    wl = 0.0;
  return wl;
}

/**
 * Find the chopper rotational frequency, in Hz
 * @param run : logs of the run
 */
void EQSANSDiskChopper::setSpeed(const API::Run &run) {
  std::ostringstream sp;
  sp << "Speed" << m_index + 1;
  typedef Mantid::Kernel::TimeSeriesProperty<double> TSeries;
  auto log = dynamic_cast<TSeries *>(run.getLogData(sp.str()));
  if (!log) {
    throw std::runtime_error("Speed log not found.");
  }
  m_speed = log->getStatistics().mean;
}

/**
 * Finds the phase of the chopper, in microseconds
 *  @param run : logs
 *  @param offset : phase between the installed sensor and the middle of the
 * opening window.
 *  @throw std::runtime_error Chopper phase reported by the sensor not found
 *  @return :: the time needed by the chopper for the middle of the transmisison
 * window to cut accross the axis defined by the neutron beam.
 */
void EQSANSDiskChopper::setPhase(const API::Run &run, double offset) {
  // Read sensor phase from the logs
  std::ostringstream ph;
  ph << "Phase"
     << m_index + 1; // log for the chopper phase reported by the sensor
  typedef Mantid::Kernel::TimeSeriesProperty<double> TSeries;
  auto log = dynamic_cast<TSeries *>(run.getLogData(ph.str()));
  if (!log) {
    throw std::runtime_error("Phase log not found.");
  }
  m_phase = log->getStatistics().mean - offset;
}

/**
 * Wavelength transmission bands allowed by one chopper.
 * Last band is cropped at maxWl if necessary.
 * @param maxWl : maximum wavelength to consider
 * @param delay : additional time-of-flight to include in the calculations
 * @param pulsed : include the correction due to delayed emission of neutrons
 * from the moderator
 */
EQSANSTransWBands EQSANSDiskChopper::transmissionBands(double maxWl,
                                                       double delay,
                                                       bool pulsed) const {
  double t_opening = rewind();
  // shortest wavelength obtained with pulsed correction, if needed
  double openingWl = tofToWavelength(t_opening, delay, pulsed);
  EQSANSTransWBands wg;
  while (openingWl < maxWl) {
    // slowest wavelength obtained with no pulse correction
    double closingWl =
        tofToWavelength(t_opening + transmissionDuration(), delay, false);
    if (closingWl > maxWl)
      closingWl = maxWl;
    wg.m_bands.emplace_back(EQSANSWBand(openingWl, closingWl));
    t_opening += period();
    openingWl = tofToWavelength(t_opening, delay, pulsed);
  }
  return wg;
}

// ######################################################
// ###############   EQSANSUnfoldFrame   ###############
// ######################################################

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSUnfoldFrame)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using Types::Event::TofEvent;

EQSANSUnfoldFrame::EQSANSUnfoldFrame()
    : API::Algorithm(), m_lowTOFcut(0.), m_highTOFcut(0.), m_pulsePeriod(0.),
      m_frameWidth(0.), m_frameOffset(0.), m_frameSkippingMode(false),
      m_choppers(NCHOPPERS) {}

/// Source pulse frequency (60Hz)
double EQSANSUnfoldFrame::getPulseFrequency() {
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto frequencyLog = dynamic_cast<TimeSeriesProperty<double> *>(
      inputWS->run().getLogData("frequency"));
  if (!frequencyLog) {
    throw std::runtime_error("Frequency log not found.");
  }
  return frequencyLog->getStatistics().mean;
}

void EQSANSUnfoldFrame::setPulsePeriod() {
  m_pulsePeriod = 1.0e6 / getPulseFrequency(); // 1.0e6/60 micro-seconds
}

/// Determine whether we need frame skipping or not by checking the speed of the
/// first chopper
void EQSANSUnfoldFrame::setFrameSkippingMode() {
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto chopperSpeedLog = dynamic_cast<TimeSeriesProperty<double> *>(
      inputWS->run().getLogData("Speed1"));
  if (!chopperSpeedLog) {
    throw std::runtime_error("Chopper speed log not found.");
  }
  const double chopperSpeed =
      chopperSpeedLog->getStatistics().mean; // 30 Hz in frame-skipping mode
  m_frameSkippingMode =
      std::fabs(chopperSpeed - getPulseFrequency() / 2.0) < 1.0 ? true : false;
}

void EQSANSUnfoldFrame::setFrameWidth() {
  m_frameWidth = m_frameSkippingMode ? m_pulsePeriod * 2.0
                                     : m_pulsePeriod; // 1.0e6/30 or 1.0e6/60 ms
}

void EQSANSUnfoldFrame::initializeChoppers() {
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto offsets =
      m_frameSkippingMode ? CHOPPER_PHASE_OFFSET[0] : CHOPPER_PHASE_OFFSET[1];
  for (size_t i = 0; i < m_choppers.size(); i++) {
    m_choppers[i].m_index = i;
    m_choppers[i].m_location = CHOPPER_LOCATION[i];
    m_choppers[i].m_aperture = CHOPPER_ANGLE[i];
    m_choppers[i].setSpeed(inputWS->run());
    m_choppers[i].setPhase(inputWS->run(), offsets[i]);
  }
}

/**
 * Find out the neutron wavelength band transmitted through all choppers.
 * Includes the time correction due to delayed emission of neutrons from the
 * moderator
 * @param delay additional time delay for neutrons originating in previous
 * pulses
 * @return wavelength band
 */
EQSANSWBand EQSANSUnfoldFrame::transmittedBand(double delay) const {
  EQSANSTransWBands g = m_choppers[0].transmissionBands(MAXWL, delay, true);
  for (size_t i = 1; i < m_choppers.size(); i++)
    g = g.intersect(m_choppers[i].transmissionBands(MAXWL, delay, true));
  if (g.size() != 1)
    throw std::runtime_error("Incorrect calculation of transmitted band");
  return g.m_bands[0];
}

/**
 * Time a neutron of a given wavelength takes to travel a certain distance
 * @param wavelength Wavelength of the neutron, in Angstroms
 * @param flightPath Distance traveled, in meters
 * @param pulsed Include the correction due to delayed emission of neutrons from
 * the moderator
 * @return Travel time, in microseconds
 */
double EQSANSUnfoldFrame::travelTime(double wavelength, double flightPath,
                                     bool pulsed) const {
  double tof = wavelength * flightPath / 3.9560346e-03;
  if (pulsed)
    tof += PULSEWIDTH * wavelength;
  return tof;
}

/**
 * Convert time of flight to neutron's wavelength.
 * @param tof time of flight, in microseconds
 * @param delay Additional time-of-flight to include in the calculations
 * @param flightPath total distance traveled by the neutron
 * @param pulsed Include the correction due to delayed emission of neutrons from
 * the moderator
 * @return neutron wavelength (in Angstroms). Returns zero for negative input
 * tof.
 */
double EQSANSUnfoldFrame::tof_to_wavelength(double tof, double flightPath,
                                            double delay, bool pulsed) const {
  double sigma = 3.9560346e-03; // plank constant divided by neutron mass
  double effective_location =
      pulsed ? (flightPath + sigma * PULSEWIDTH) : flightPath;
  double wl = sigma * (tof + delay) / effective_location;
  if (wl < 0.0)
    wl = 0.0;
  return wl;
}

/**
 * Distance from moderator to the center of the detector array
 * @throw std::runtime_error if property "sample_detector_distance" not found in
 * the logs
 */
double EQSANSUnfoldFrame::nominalDetectorDistance() const {
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const auto &spectrumInfo = inputWS->spectrumInfo();
  double moderatorToSampleDistance = spectrumInfo.l1();
  Mantid::Kernel::Property *prop =
      inputWS->run().getProperty("sample_detector_distance");
  auto dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  if (!dp)
    throw std::runtime_error(
        "Property sample_detector_distance not found in the logs. "
        "Maybe workspace was not generated with EQSANSLoad?");
  return (*dp / 1000.0) + moderatorToSampleDistance;
}

/**
 * Find the frame we are operating using the nominal detector distance
 * @param wavelength Wavelength of the traveling neutron
 * @returns 0 if operating in the first frame, 1 if in the second frame, and so
 * on
 */
size_t EQSANSUnfoldFrame::frameOperating(double wavelength) const {
  double tof = travelTime(wavelength, nominalDetectorDistance());
  return static_cast<size_t>(std::floor(tof / m_frameWidth));
}

void EQSANSUnfoldFrame::init() {
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Workspace to apply the TOF correction to");
  declareProperty("LowTOFCut", 0.0,
                  "Width of the TOF margin to clip the lower end of the TOF "
                  "distribution for each frame. Units of"
                  "micro-seconds",
                  Kernel::Direction::Input);
  declareProperty("HighTOFCut", 0.0,
                  "Width of the TOF margin to clip the upper end of the TOF "
                  "distribution of each frame. Cannot"
                  "be larger than 1.0E06/60 micro-seconds",
                  Kernel::Direction::Input);

  // Output parameters
  declareProperty("FrameSkipping", false,
                  "If True, the data was taken in frame skipping mode",
                  Kernel::Direction::Output);
  declareProperty("TofOffset", 0.0, "TOF offset that was applied to the data",
                  Kernel::Direction::Output);
  declareProperty(
      "WavelengthMin", 0.0,
      "Lower bound of the wavelength distribution of the first frame",
      Kernel::Direction::Output);
  declareProperty(
      "WavelengthMax", 0.0,
      "Upper bound of the wavelength distribution of the first frame",
      Kernel::Direction::Output);
  // Wavelength distributions for the second frame only when in frame skipping
  // mode
  declareProperty(
      "WavelengthMinFrame2", 0.0,
      "Lower bound of the wavelength distribution of the second frame",
      Kernel::Direction::Output);
  declareProperty(
      "WavelengthMaxFrame2", 0.0,
      "Upper bound of the wavelength distribution of the second frame",
      Kernel::Direction::Output);
}

void EQSANSUnfoldFrame::exec() {
  // collect input properties
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  m_lowTOFcut = getProperty("LowTOFCut");
  m_highTOFcut = getProperty("HighTOFCut");

  // Properties for all detectors
  setPulsePeriod();       // initialize m_pulsePeriod
  setFrameSkippingMode(); // initialize m_frameSkippingMode
  setFrameWidth();        // initialize m_frameWidth
  initializeChoppers();   // Generate and initialize the disk chopper objects

  this->execEvent();
}

void EQSANSUnfoldFrame::execEvent() {

  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS)
    throw std::runtime_error("Input workspace is not an EventWorkspace");

  const auto &spectrumInfo = inputWS->spectrumInfo();
  double moderatorToSampleDistance = spectrumInfo.l1();
  double nominalPath = nominalDetectorDistance();

  // Neutron wavelength band transmitted through the choppers for the leading
  // frame
  EQSANSWBand transBand = transmittedBand();
  // Neutron wavelength band transmitted through the choppers for the skipped
  // frame
  EQSANSWBand transBandSk = m_frameSkippingMode ? transmittedBand(m_pulsePeriod)
                                                : EQSANSWBand(0.0, 0.0);
  // Find the offset time if we are not operating in the first frame
  /// m_frameOffset is zero when operating in the first frame
  m_frameOffset =
      static_cast<double>(frameOperating(transBand.m_min)) * m_frameWidth;

  // Loop through the spectra and apply correction
  const size_t numHists = inputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHists);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS))
  for (int64_t ispec = 0; ispec < int64_t(numHists); ++ispec) {
    PARALLEL_START_INTERUPT_REGION

    if (!spectrumInfo.hasDetectors(ispec)) {
      g_log.warning()
          << "Workspace index " << ispec
          << " has no detector assigned to eventsIterator - discarding\n";
      continue;
    }

    double sampleToDetectorDistance = spectrumInfo.l2(ispec);
    double flightPath = moderatorToSampleDistance + sampleToDetectorDistance;
    // Shortest and longest TOF for neutrons originating in the leading frame
    double sTof = travelTime(transBand.m_min, flightPath, true);
    double lTof = travelTime(transBand.m_max, flightPath);
    // Shortest and longest TOF for neutrons originating in the skipped frame
    double sTofSk = m_frameSkippingMode
                        ? travelTime(transBandSk.m_min, flightPath, true)
                        : 0.0;
    double lTofSk =
        m_frameSkippingMode ? travelTime(transBandSk.m_max, flightPath) : 0.0;

    // Only neutrons arriving at the nominal detector have no overlap. However,
    // there is overlap for all other detectors because their flight paths are
    // longer.
    double frame_overlap =
        m_pulsePeriod * (flightPath - nominalPath) / nominalPath;
    // The skipped pulse originates one pulse period behind, thus 2 *
    // m_pulsePeriod
    double frame_overlapSk = 2 * frame_overlap; // overlap for the skipped pulse

    // Get the pointer to the output event list
    std::vector<TofEvent> &events = inputWS->getSpectrum(ispec).getEvents();
    std::vector<TofEvent>::iterator eventsIterator;
    std::vector<TofEvent> correctedEvents;

    for (eventsIterator = events.begin(); eventsIterator < events.end();
         ++eventsIterator) {

      // Shift TOF to the correct frame (first, second, third, or fourth frame)
      double newTOF = eventsIterator->tof();
      newTOF += m_frameOffset;

      // Slow neutrons that were assigned a TOF smaller than that of the fastest
      // neutrons must be shifted by the TOF frame width
      if (newTOF < sTof)
        newTOF += m_frameWidth;

      // if the neutron originated at the skipped pulse, its TOF must be
      // increased by a pulse period
      if (m_frameSkippingMode && newTOF > lTof)
        newTOF += m_pulsePeriod;

      // Discard event if TOF within any of the overlapping-frame regions
      if (newTOF < sTof + frame_overlap)
        continue; // event within the long-time frame-overlap region
      if (newTOF > lTof - frame_overlap && newTOF < lTof)
        continue; // event within the long-time frame-overlap region
      if (m_frameSkippingMode) {
        if (newTOF > sTofSk && newTOF < sTofSk + frame_overlapSk)
          continue;
        if (newTOF > lTofSk - frame_overlapSk)
          continue;

        // Optionally, discard event if falling within the input TOF cuts
        if (m_lowTOFcut > 0.0) {
          if (newTOF < sTof + m_lowTOFcut)
            continue;
          if (newTOF > sTofSk && newTOF < sTofSk + m_lowTOFcut)
            continue;
          if (m_highTOFcut > 0.0) {
            if (newTOF > lTof - m_highTOFcut && newTOF < lTof)
              continue;
            if (newTOF > lTofSk - m_highTOFcut)
              continue;
          }
        }
      }
      correctedEvents.emplace_back(newTOF, eventsIterator->pulseTime());
    }

    // Replace contents of list events with that of correctedEvents
    events.clear();
    events.reserve(correctedEvents.size());
    for (eventsIterator = correctedEvents.begin();
         eventsIterator < correctedEvents.end(); ++eventsIterator)
      events.push_back(*eventsIterator);

    progress.report("TOF structure");
    PARALLEL_END_INTERUPT_REGION
  } // ispec == numHists
  PARALLEL_CHECK_INTERUPT_REGION
} // EQSANSUnfoldFrame::execevent()

} // namespace Algorithms
} // namespace Mantid
