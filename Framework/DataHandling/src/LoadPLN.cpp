// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadPLN.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadANSTOEventFile.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <utility>

namespace Mantid::DataHandling {

using namespace Kernel;

namespace {

// number of physical detectors
constexpr size_t MONITORS = 8;
constexpr size_t DETECTOR_TUBES = 200;
constexpr size_t HISTO_BINS_X = DETECTOR_TUBES + MONITORS;
constexpr size_t HISTO_BINS_Y_DENUMERATOR = 16;
constexpr size_t PIXELS_PER_TUBE = 1024 / HISTO_BINS_Y_DENUMERATOR;
constexpr size_t DETECTOR_SPECTRA = DETECTOR_TUBES * PIXELS_PER_TUBE;
constexpr size_t HISTOGRAMS = DETECTOR_SPECTRA + MONITORS;

// File loading progress boundaries
constexpr size_t Progress_LoadBinFile = 48;
constexpr size_t Progress_ReserveMemory = 4;
constexpr size_t Progress_Total = 2 * Progress_LoadBinFile + Progress_ReserveMemory;

// Algorithm parameter names
constexpr char FilenameStr[] = "Filename";
constexpr char MaskStr[] = "Mask";
constexpr char SelectDetectorTubesStr[] = "SelectDetectorTubes";
constexpr char SelectDatasetStr[] = "SelectDataset";
constexpr char FilterByTimeStartStr[] = "FilterByTimeStart";
constexpr char FilterByTimeStopStr[] = "FilterByTimeStop";
constexpr char PathToBinaryStr[] = "BinaryEventPath";
constexpr char TOFBiasStr[] = "TimeOfFlightBias";
constexpr char CalibrateTOFStr[] = "CalibrateTOFBias";
constexpr char LambdaOnTwoStr[] = "LambdaOnTwoMode";

// Common pairing of limits
using TimeLimits = std::pair<double, double>;

template <typename Type>
void AddSinglePointTimeSeriesProperty(API::LogManager &logManager, const std::string &time, const std::string &name,
                                      const Type value) {
  // create time series property and add single value
  auto p = new Kernel::TimeSeriesProperty<Type>(name);
  p->addValue(time, value);

  // add to log manager
  logManager.addProperty(p);
}

// Utility functions for loading values with defaults
// Single value properties only support int, double, string and bool
template <typename Type>
Type GetNeXusValue(const NeXus::NXEntry &entry, const std::string &path, const Type &defval, int32_t index) {
  try {
    NeXus::NXDataSetTyped<Type> dataSet = entry.openNXDataSet<Type>(path);
    dataSet.load();

    return dataSet()[index];
  } catch (std::runtime_error &) {
    return defval;
  }
}

// string and double are special cases
template <>
double GetNeXusValue<double>(const NeXus::NXEntry &entry, const std::string &path, const double &defval,
                             int32_t index) {
  try {
    NeXus::NXFloat dataSet = entry.openNXDataSet<float>(path);
    dataSet.load();

    return dataSet()[index];
  } catch (std::runtime_error &) {
    return defval;
  }
}

template <>
std::string GetNeXusValue<std::string>(const NeXus::NXEntry &entry, const std::string &path, const std::string &defval,
                                       int32_t /*unused*/) {

  try {
    NeXus::NXChar dataSet = entry.openNXChar(path);
    dataSet.load();

    return std::string(dataSet(), dataSet.dim0());
  } catch (std::runtime_error &) {
    return defval;
  }
}

template <typename T>
void MapNeXusToProperty(const NeXus::NXEntry &entry, const std::string &path, const T &defval,
                        API::LogManager &logManager, const std::string &name, const T &factor, int32_t index) {

  T value = GetNeXusValue<T>(entry, path, defval, index);
  logManager.addProperty<T>(name, value * factor);
}

// string is a special case
template <>
void MapNeXusToProperty<std::string>(const NeXus::NXEntry &entry, const std::string &path, const std::string &defval,
                                     API::LogManager &logManager, const std::string &name,
                                     const std::string & /*unused*/, int32_t index) {

  std::string value = GetNeXusValue<std::string>(entry, path, defval, index);
  logManager.addProperty<std::string>(name, value);
}

template <typename T>
void MapNeXusToSeries(const NeXus::NXEntry &entry, const std::string &path, const T &defval,
                      API::LogManager &logManager, const std::string &time, const std::string &name, const T &factor,
                      int32_t index) {

  auto value = GetNeXusValue<T>(entry, path, defval, index);
  AddSinglePointTimeSeriesProperty<T>(logManager, time, name, value * factor);
}

// map the comma separated range of indexes to the vector via a lambda function
// throws an exception if it is outside the vector range
//
template <typename T, typename F> void mapRangeToIndex(const std::string &line, std::vector<T> &result, const F &fn) {

  std::stringstream ss(line);
  std::string item;
  size_t index = 0;
  while (std::getline(ss, item, ',')) {
    auto const k = item.find('-');

    size_t p0, p1;
    if (k != std::string::npos) {
      p0 = boost::lexical_cast<size_t>(item.substr(0, k));
      p1 = boost::lexical_cast<size_t>(item.substr(k + 1, item.size() - k - 1));
    } else {
      p0 = boost::lexical_cast<size_t>(item);
      p1 = p0;
    }

    if (p1 < result.size() && p0 <= p1) {
      while (p0 <= p1) {
        result[p0++] = fn(index);
        index++;
      }
    } else if (p0 < result.size() && p1 < p0) {
      do {
        result[p0] = fn(index);
        index++;
      } while (p1 < p0--);
    } else
      throw std::invalid_argument("invalid range specification");
  }
}

// Simple reader that is compatible with the ASNTO event file loader
class FileLoader {
  std::ifstream _ifs;
  size_t _size;

public:
  explicit FileLoader(const char *filename) : _ifs(filename, std::ios::binary | std::ios::in) {
    if (!_ifs.is_open() || _ifs.fail())
      throw std::runtime_error("unable to open file");

    _ifs.seekg(0, _ifs.end);
    _size = _ifs.tellg();
    _ifs.seekg(0, _ifs.beg);
  }

  bool read(char *s, std::streamsize n) { return static_cast<bool>(_ifs.read(s, n)); }

  size_t size() const { return _size; }

  size_t position() { return _ifs.tellg(); }

  size_t selected_position() { return _ifs.tellg(); }
};

} // anonymous namespace

namespace PLN {

//
// In the future the ANSTO helper and event file loader will be generalized to
// handle the instruments consistently.

// Simple 1D histogram class
class SimpleHist {
  std::vector<size_t> m_hist;
  double m_M;
  double m_B;
  size_t m_peak;
  size_t m_count;

public:
  SimpleHist(size_t N, double minVal, double maxVal) : m_hist(N, 0) {
    m_M = (static_cast<double>(N) / (maxVal - minVal));
    m_B = -m_M * minVal;
    m_peak = 0;
    m_count = 0;
  }

  inline double ival(double val) const { return m_M * val + m_B; }

  inline double xval(double ix) const { return (ix - m_B) / m_M; }

  inline void add(double val) {
    auto ix = static_cast<size_t>(std::floor(ival(val)));
    if (ix < m_hist.size()) {
      m_hist[ix]++;
      m_count++;
      if (m_hist[ix] > m_peak)
        m_peak = m_hist[ix];
    }
  }

  const std::vector<size_t> &histogram() const { return m_hist; }

  inline size_t peak() const { return m_peak; }
  inline size_t count() const { return m_count; }
};

class EventProcessor {
protected:
  // fields
  const std::vector<bool> &m_roi;
  const std::vector<size_t> &m_mapIndex;
  const double m_framePeriod;
  const double m_gatePeriod;

  // number of frames
  size_t m_frames;
  size_t m_framesValid;

  // number of events
  size_t m_maxEvents;
  size_t m_processedEvents;
  size_t m_droppedEvents;

  // time boundaries
  const TimeLimits m_timeBoundary; // seconds

  virtual void addEventImpl(size_t id, size_t x, size_t y, double tof) = 0;

public:
  EventProcessor(const std::vector<bool> &roi, const std::vector<size_t> &mapIndex, const double framePeriod,
                 const double gatePeriod, const TimeLimits &timeBoundary, size_t maxEvents)
      : m_roi(roi), m_mapIndex(mapIndex), m_framePeriod(framePeriod), m_gatePeriod(gatePeriod), m_frames(0),
        m_framesValid(0), m_maxEvents(maxEvents), m_processedEvents(0), m_droppedEvents(0),
        m_timeBoundary(timeBoundary) {}

  void newFrame() {
    if (m_maxEvents == 0 || m_processedEvents < m_maxEvents) {
      m_frames++;
      if (validFrame())
        m_framesValid++;
    }
  }

  inline bool validFrame() const {
    double frameTime = m_framePeriod * static_cast<double>(m_frames) * 1.0e-6;
    return (frameTime >= m_timeBoundary.first && frameTime <= m_timeBoundary.second);
  }

  double duration() const {
    // length test in seconds
    return m_framePeriod * static_cast<double>(m_frames) * 1.0e-6;
  }

  inline int64_t frameStart() const {
    // returns time in nanoseconds from start of test
    auto start = m_framePeriod * static_cast<double>(m_frames);
    return static_cast<int64_t>(start * 1.0e3);
  }

  size_t numFrames() const { return m_framesValid; }

  size_t availableEvents() const { return m_processedEvents + m_droppedEvents; }

  size_t processedEvents() const { return m_processedEvents; }

  void addEvent(size_t x, size_t p, double tof, double /*taux*/) {

    // check if in time boundaries
    if (!validFrame())
      return;

    // group pixels
    auto y = static_cast<size_t>(p / HISTO_BINS_Y_DENUMERATOR);

    // determine detector id and check limits
    if (x >= HISTO_BINS_X || y >= PIXELS_PER_TUBE)
      return;

    // map the raw detector index to the physical model
    size_t xid = m_mapIndex[x];

    size_t id = xid < DETECTOR_TUBES ? PIXELS_PER_TUBE * xid + y : DETECTOR_SPECTRA + xid;
    if (id >= m_roi.size())
      return;

    // check if neutron is in region of interest
    if (!m_roi[id])
      return;

    // finally pass to specific handler
    if (m_maxEvents == 0 || m_processedEvents < m_maxEvents) {
      // take the modulus of the tof time to account for the
      // longer background chopper rate
      double mtof = tof < 0.0 ? fmod(tof + m_gatePeriod, m_gatePeriod) : fmod(tof, m_gatePeriod);

      addEventImpl(id, xid, y, mtof);
      m_processedEvents++;
    } else {
      m_droppedEvents++;
    }
  }
};

// The class determines the number of counts linked to the detectors and the
// tof correction.
class EventCounter : public EventProcessor {
protected:
  // fields
  std::vector<size_t> &m_eventCounts;
  double m_L1;
  double m_V0;
  const std::vector<double> &m_L2;
  SimpleHist m_histogram;

  void addEventImpl(size_t id, size_t /*x*/, size_t /*y*/, double tobs) override {
    m_eventCounts[id]++;
    // the maximum occurs at the elastic peak
    double deltaT = 1.0e6 * (m_L1 + m_L2[id]) / m_V0 - tobs;
    m_histogram.add(deltaT);
  }

public:
  // construction
  EventCounter(const std::vector<bool> &roi, const std::vector<size_t> &mapIndex, const double framePeriod,
               const double gatePeriod, const TimeLimits &timeBoundary, std::vector<size_t> &eventCounts,
               const double L1, const double V0, const std::vector<double> &vecL2, size_t maxEvents)
      : EventProcessor(roi, mapIndex, framePeriod, gatePeriod, timeBoundary, maxEvents), m_eventCounts(eventCounts),
        m_L1(L1), m_V0(V0), m_L2(vecL2), m_histogram(5000, -2500.0, 2500.0) {}

  // clips the histogram above 25% and takes the mean of the values
  double tofCorrection() {

    // determine the points above the 25% threshold
    auto minLevel = static_cast<size_t>(m_histogram.peak() / 4);
    auto hvec = m_histogram.histogram();
    double sum = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < hvec.size(); i++) {
      if (hvec[i] >= minLevel) {
        auto ix = static_cast<double>(i);
        sum += static_cast<double>(hvec[i]) * m_histogram.xval(ix + 0.5);
        count += hvec[i];
      }
    }

    return (count > 0 ? sum / static_cast<double>(count) : 0.0);
  }
};

class EventAssigner : public EventProcessor {
protected:
  // fields
  std::vector<EventVector_pt> &m_eventVectors;
  double m_tofMin;
  double m_tofMax;
  int64_t m_startTime;
  double m_tofCorrection;
  double m_sampleTime;

  void addEventImpl(size_t id, size_t /*x*/, size_t /*y*/, double tobs) override {

    // get the absolute time for the start of the frame
    auto const offset = m_startTime + frameStart();

    // adjust the tof to account for the correction and allocate events
    // that occur before the sample time as slow events from the previous pulse
    double tof = tobs + m_tofCorrection - m_sampleTime;
    if (tof < 0.0)
      tof = fmod(tof + m_gatePeriod, m_gatePeriod);
    tof += m_sampleTime;
    if (m_tofMin > tof)
      m_tofMin = tof;
    if (m_tofMax < tof)
      m_tofMax = tof;

    auto ev = Types::Event::TofEvent(tof, Types::Core::DateAndTime(offset));
    m_eventVectors[id]->emplace_back(ev);
  }

public:
  EventAssigner(const std::vector<bool> &roi, const std::vector<size_t> &mapIndex, const double framePeriod,
                const double gatePeriod, const TimeLimits &timeBoundary, std::vector<EventVector_pt> &eventVectors,
                int64_t startTime, double tofCorrection, double sampleTime, size_t maxEvents)
      : EventProcessor(roi, mapIndex, framePeriod, gatePeriod, timeBoundary, maxEvents), m_eventVectors(eventVectors),
        m_tofMin(std::numeric_limits<double>::max()), m_tofMax(std::numeric_limits<double>::min()),
        m_startTime(startTime), m_tofCorrection(tofCorrection), m_sampleTime(sampleTime) {}

  double tofMin() const { return m_tofMin <= m_tofMax ? m_tofMin : 0.0; }
  double tofMax() const { return m_tofMin <= m_tofMax ? m_tofMax : 0.0; }
};

template <typename EP>
void loadEvents(API::Progress &prog, const char *progMsg, const std::string &eventFile, EP &eventProcessor) {

  using namespace ANSTO;

  prog.doReport(progMsg);

  FileLoader loader(eventFile.c_str());

  // for progress notifications
  ANSTO::ProgressTracker progTracker(prog, progMsg, loader.size(), Progress_LoadBinFile);

  ReadEventFile(loader, eventProcessor, progTracker, 100, false);
}
} // namespace PLN

/// Initialise the algorithm and declare the properties for the
/// nexus descriptor.
void LoadPLN::init() {

  // Specify file extensions which can be associated with a specific file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.emplace_back(".hdf");
  declareProperty(std::make_unique<API::FileProperty>(FilenameStr, "", API::FileProperty::Load, exts),
                  "The input filename of the stored data");

  declareProperty(PathToBinaryStr, "./", std::make_shared<Kernel::MandatoryValidator<std::string>>(),
                  "Relative or absolute path to the compressed binary\n"
                  "event file linked to the HDF file, eg /storage/data/");

  // mask
  exts.clear();
  exts.emplace_back(".xml");
  declareProperty(std::make_unique<API::FileProperty>(MaskStr, "", API::FileProperty::OptionalLoad, exts),
                  "The input filename of the mask data");

  declareProperty(SelectDetectorTubesStr, "",
                  "Comma separated range of detectors tubes to be loaded,\n"
                  "  eg 16,19-45,47");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::IEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output));

  declareProperty(SelectDatasetStr, 0, "Select the index for the dataset to be loaded.");

  declareProperty(TOFBiasStr, 0.0, "Time of flight correction in micro-sec.");

  declareProperty(CalibrateTOFStr, false, "Calibrate the TOF correction from the elastic pulse.");

  declareProperty(LambdaOnTwoStr, false, "Instrument is operating in Lambda on Two mode.");

  declareProperty(FilterByTimeStartStr, 0.0,
                  "Only include events after the provided start time, in "
                  "seconds (relative to the start of the run).");

  declareProperty(FilterByTimeStopStr, EMPTY_DBL(),
                  "Only include events before the provided stop time, in "
                  "seconds (relative to the start of the run).");

  std::string grpOptional = "Filters";
  setPropertyGroup(FilterByTimeStartStr, grpOptional);
  setPropertyGroup(FilterByTimeStopStr, grpOptional);
}

/// Creates an event workspace and sets the \p title.
void LoadPLN::createWorkspace(const std::string &title) {

  // Create the workspace
  m_localWorkspace = std::make_shared<DataObjects::EventWorkspace>();
  m_localWorkspace->initialize(HISTOGRAMS, 2, 1);

  // set the units
  m_localWorkspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnit("Counts");

  // set title
  m_localWorkspace->setTitle(title);
}

/// Execute the algorithm using the \p hdfFile and \p eventFile.
/// The steps involved are:
///   Create the workspace
///   Get the instrument properties and load options
///   Load the instrument from the IDF
///   Load the data values and adjust TOF
///   Set up the masks

void LoadPLN::exec(const std::string &hdfFile, const std::string &eventFile) {

  namespace fs = std::filesystem;

  // Create workspace
  // ----------------
  fs::path p = hdfFile;
  for (; !p.extension().empty();)
    p = p.stem();
  std::string title = p.generic_string();
  createWorkspace(title);
  API::LogManager &logManager = m_localWorkspace->mutableRun();
  API::Progress prog(this, 0.0, 1.0, Progress_Total);

  // Load instrument and workspace properties
  logManager.addProperty(SelectDatasetStr, m_datasetIndex);
  loadParameters(hdfFile, logManager);
  prog.doReport("creating instrument");
  loadInstrument();

  // Get the region of interest and filters and save to log
  std::string const maskfile = getPropertyValue(MaskStr);
  std::string const seltubes = getPropertyValue(SelectDetectorTubesStr);
  logManager.addProperty(SelectDetectorTubesStr, seltubes);
  logManager.addProperty(MaskStr, maskfile);

  std::vector<bool> roi = createRoiVector(seltubes, maskfile);
  double timeMaxBoundary = getProperty(FilterByTimeStopStr);
  if (isEmpty(timeMaxBoundary))
    timeMaxBoundary = std::numeric_limits<double>::infinity();
  TimeLimits timeBoundary(getProperty(FilterByTimeStartStr), timeMaxBoundary);

  // get the detector map from raw input to a physical detector
  auto instr = m_localWorkspace->getInstrument();
  std::string dmapStr = instr->getParameterAsString("DetectorMap");
  std::vector<size_t> detMapIndex = std::vector<size_t>(HISTO_BINS_X, 0);
  mapRangeToIndex(dmapStr, detMapIndex, [](size_t n) { return n; });

  // Load the events file. First count the number of events to reserve
  // memory and then assign the events to the detectors
  size_t numberHistograms = m_localWorkspace->getNumberHistograms();
  std::vector<EventVector_pt> eventVectors(numberHistograms, nullptr);
  std::vector<size_t> eventCounts(numberHistograms, 0);

  double masterRpm = fabs(logManager.getTimeSeriesProperty<double>("FermiChopperFreq")->firstValue());
  double slaveRpm = fabs(logManager.getTimeSeriesProperty<double>("OverlapChopperFreq")->firstValue());
  double framePeriod = 1.0e6 / masterRpm;

  // if fermi chopper freq equals the overlap freq then the gate period is
  // half the frame period
  double gatePeriod = (std::round(masterRpm / slaveRpm) == 1.0 ? 0.5 * framePeriod : framePeriod);
  AddSinglePointTimeSeriesProperty<double>(logManager, m_startRun, "GatePeriod", gatePeriod);

  // count total events per pixel and reserve necessary memory
  size_t hdfCounts = static_cast<size_t>(logManager.getTimeSeriesProperty<int>("TotalCounts")->firstValue());
  loadDetectorL2Values();
  double sourceSample = fabs(instr->getSource()->getPos().Z());
  double wavelength = logManager.getTimeSeriesProperty<double>("Wavelength")->firstValue();
  double velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass * wavelength * 1e-10);
  double sampleTime = 1.0e6 * sourceSample / velocity;
  PLN::EventCounter eventCounter(roi, detMapIndex, framePeriod, gatePeriod, timeBoundary, eventCounts, sourceSample,
                                 velocity, m_detectorL2, hdfCounts);
  PLN::loadEvents(prog, "loading neutron counts", eventFile, eventCounter);
  ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists", numberHistograms, Progress_ReserveMemory);
  prepareEventStorage(progTracker, eventCounts, eventVectors);

  // log a message if the number of events in the event file does not match
  // the total counts in the hdf
  if (hdfCounts != eventCounter.availableEvents()) {
    g_log.error("HDF and event counts differ: " + std::to_string(hdfCounts) + ", " +
                std::to_string(eventCounter.availableEvents()));
  }

  // now perform the actual event collection and TOF convert if necessary
  // if a phase calibration is required then load it as raw doppler time
  // perform the calibration and then convert to TOF
  Types::Core::DateAndTime startTime(m_startRun);
  auto const start_nanosec = startTime.totalNanoseconds();
  bool const calibrateTOF = getProperty(CalibrateTOFStr);
  double tofCorrection = getProperty(TOFBiasStr);
  if (calibrateTOF) {
    tofCorrection = eventCounter.tofCorrection();
  }
  logManager.addProperty("CalibrateTOF", (calibrateTOF ? 1 : 0));
  AddSinglePointTimeSeriesProperty<double>(logManager, m_startRun, "TOFCorrection", tofCorrection);
  PLN::EventAssigner eventAssigner(roi, detMapIndex, framePeriod, gatePeriod, timeBoundary, eventVectors, start_nanosec,
                                   tofCorrection, sampleTime, hdfCounts);
  PLN::loadEvents(prog, "loading neutron events (TOF)", eventFile, eventAssigner);

  // perform a calibration and then TOF conversion if necessary
  // and update the tof limits
  auto minTOF = eventAssigner.tofMin();
  auto maxTOF = eventAssigner.tofMax();

  // just to make sure the bins hold it all and setup the detector masks
  m_localWorkspace->setAllX(HistogramData::BinEdges{std::max(0.0, floor(minTOF)), maxTOF + 1});
  setupDetectorMasks(roi);

  // set log values
  auto frame_count = static_cast<int>(eventCounter.numFrames());
  AddSinglePointTimeSeriesProperty<int>(logManager, m_startRun, "frame_count", frame_count);

  std::string filename = getPropertyValue(FilenameStr);
  logManager.addProperty("filename", filename);

  Types::Core::time_duration duration =
      boost::posix_time::microseconds(static_cast<boost::int64_t>(eventCounter.duration() * 1.0e6));
  Types::Core::DateAndTime endTime(startTime + duration);
  logManager.addProperty("start_time", startTime.toISO8601String());
  logManager.addProperty("end_time", endTime.toISO8601String());
  logManager.addProperty<double>("dur", eventCounter.duration());

  // Finally add the time-series evironment parameters explicitly
  loadEnvironParameters(hdfFile, logManager);

  setProperty("OutputWorkspace", m_localWorkspace);
}

/// Recovers the L2 neutronic distance for each detector.
void LoadPLN::loadDetectorL2Values() {

  m_detectorL2.resize(HISTOGRAMS, 0.0);
  const auto &detectorInfo = m_localWorkspace->detectorInfo();
  auto detIDs = detectorInfo.detectorIDs();
  for (const auto detID : detIDs) {
    auto ix = detectorInfo.indexOf(detID);
    double l2 = detectorInfo.l2(ix);
    m_detectorL2[detID] = l2;
  }
}

/// Set up the detector masks to the region of interest \p roi.
void LoadPLN::setupDetectorMasks(const std::vector<bool> &roi) {

  // count total number of masked bins
  size_t maskedBins = 0;
  for (size_t i = 0; i != roi.size(); i++)
    if (!roi[i])
      maskedBins++;

  if (maskedBins > 0) {
    // create list of masked bins
    std::vector<size_t> maskIndexList(maskedBins);
    size_t maskIndex = 0;

    for (size_t i = 0; i != roi.size(); i++)
      if (!roi[i])
        maskIndexList[maskIndex++] = i;

    auto maskingAlg = createChildAlgorithm("MaskDetectors");
    maskingAlg->setProperty("Workspace", m_localWorkspace);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }
}

/// Allocate space for the event storage in \p eventVectors after the
/// \p eventCounts have been determined.
void LoadPLN::prepareEventStorage(ANSTO::ProgressTracker &progTracker, std::vector<size_t> &eventCounts,
                                  std::vector<EventVector_pt> &eventVectors) {

  size_t numberHistograms = eventCounts.size();
  for (size_t i = 0; i != numberHistograms; ++i) {
    DataObjects::EventList &eventList = m_localWorkspace->getSpectrum(i);

    eventList.setSortOrder(DataObjects::PULSETIME_SORT);
    eventList.reserve(eventCounts[i]);

    eventList.setDetectorID(static_cast<detid_t>(i));
    eventList.setSpectrumNo(static_cast<detid_t>(i));

    DataObjects::getEventsFrom(eventList, eventVectors[i]);

    progTracker.update(i);
  }
  progTracker.complete();
}

/// Region of interest is defined by the \p selected detectors and the
/// \p maskfile.
std::vector<bool> LoadPLN::createRoiVector(const std::string &selected, const std::string &maskfile) {

  std::vector<bool> result(HISTOGRAMS, true);

  // turn off pixels linked to missing tubes
  if (!selected.empty()) {
    std::vector<bool> tubes(HISTO_BINS_X, false);
    mapRangeToIndex(selected, tubes, [](size_t) { return true; });
    for (size_t i = 0; i < DETECTOR_TUBES; i++) {
      if (tubes[i] == false) {
        for (size_t j = 0; j < PIXELS_PER_TUBE; j++) {
          result[i * PIXELS_PER_TUBE + j] = false;
        }
      }
    }
    for (size_t i = 0; i < MONITORS; i++) {
      result[DETECTOR_SPECTRA + i] = tubes[DETECTOR_TUBES + i];
    }
  }

  if (maskfile.length() == 0)
    return result;

  std::ifstream input(maskfile.c_str());
  if (!input.good())
    throw std::invalid_argument("invalid mask file");

  std::string line;
  while (std::getline(input, line)) {
    auto i0 = line.find("<detids>");
    auto iN = line.find("</detids>");

    if ((i0 != std::string::npos) && (iN != std::string::npos) && (i0 < iN)) {
      line = line.substr(i0 + 8, iN - i0 - 8); // 8 = len("<detids>")
      mapRangeToIndex(line, result, [](size_t) { return false; });
    }
  }

  return result;
}

/// Load parameters from input \p hdfFile and save to the log manager, \p logm.
void LoadPLN::loadParameters(const std::string &hdfFile, API::LogManager &logm) {

  NeXus::NXRoot root(hdfFile);
  NeXus::NXEntry entry = root.openFirstEntry();

  MapNeXusToProperty<std::string>(entry, "sample/name", "unknown", logm, "SampleName", "", 0);
  MapNeXusToProperty<std::string>(entry, "sample/description", "unknown", logm, "SampleDescription", "", 0);

  // if dataset index > 0 need to add an offset to the start time
  Types::Core::DateAndTime startTime(GetNeXusValue<std::string>(entry, "start_time", "2000-01-01T00:00:00", 0));
  if (m_datasetIndex > 0) {
    auto baseTime = GetNeXusValue<int32_t>(entry, "instrument/detector/start_time", 0, 0);
    auto nthTime = GetNeXusValue<int32_t>(entry, "instrument/detector/start_time", 0, m_datasetIndex);

    Types::Core::time_duration duration =
        boost::posix_time::microseconds((static_cast<int64_t>(nthTime) - static_cast<int64_t>(baseTime)) * 1'000'000);
    Types::Core::DateAndTime startDataset(startTime + duration);
    m_startRun = startDataset.toISO8601String();
  } else {
    m_startRun = startTime.toISO8601String();
  }

  // Add support for instrument running in lambda on two mode.
  // Added as UI option as the available instrument parameters
  // cannot be reliably interpreted to predict the mode (as
  // advised by the instrument scientist).
  bool const lambdaOnTwoMode = getProperty(LambdaOnTwoStr);
  double lambdaFactor = (lambdaOnTwoMode ? 0.5 : 1.0);
  logm.addProperty("LambdaOnTwoMode", (lambdaOnTwoMode ? 1 : 0));

  MapNeXusToSeries<double>(entry, "instrument/fermi_chopper/mchs", 0.0, logm, m_startRun, "FermiChopperFreq", 1.0 / 60,
                           m_datasetIndex);
  MapNeXusToSeries<double>(entry, "instrument/fermi_chopper/schs", 0.0, logm, m_startRun, "OverlapChopperFreq",
                           1.0 / 60, m_datasetIndex);
  MapNeXusToSeries<double>(entry, "instrument/crystal/wavelength", 0.0, logm, m_startRun, "Wavelength", lambdaFactor,
                           m_datasetIndex);
  MapNeXusToSeries<double>(entry, "instrument/detector/stth", 0.0, logm, m_startRun, "DetectorTankAngle", 1.0,
                           m_datasetIndex);
  MapNeXusToSeries<int32_t>(entry, "monitor/bm1_counts", 0, logm, m_startRun, "MonitorCounts", 1, m_datasetIndex);
  MapNeXusToSeries<int32_t>(entry, "data/total_counts", 0, logm, m_startRun, "TotalCounts", 1, m_datasetIndex);
  MapNeXusToSeries<double>(entry, "data/tofw", 5.0, logm, m_startRun, "ChannelWidth", 1, m_datasetIndex);
  MapNeXusToSeries<double>(entry, "sample/mscor", 0.0, logm, m_startRun, "SampleRotation", 1, m_datasetIndex);
}

/// Load the environment variables from the \p hdfFile and save as
/// time series to the log manager, \p logm.
void LoadPLN::loadEnvironParameters(const std::string &hdfFile, API::LogManager &logm) {

  NeXus::NXRoot root(hdfFile);
  NeXus::NXEntry entry = root.openFirstEntry();
  auto time_str = logm.getPropertyValueAsType<std::string>("end_time");

  // load the environment variables for the dataset loaded
  auto tags = ANSTO::filterDatasets(entry, "data/", "^[A-Z]{1,3}[0-9]{1,3}[A-Z]{1,3}[0-9]{1,3}$");
  for (const auto &tag : tags) {
    MapNeXusToSeries<double>(entry, "data/" + tag, 0.0, logm, time_str, "env_" + tag, 1.0, m_datasetIndex);
  }
}

/// Load the instrument definition.
void LoadPLN::loadInstrument() {

  // loads the IDF and parameter file
  auto loadInstrumentAlg = createChildAlgorithm("LoadInstrument");
  loadInstrumentAlg->setProperty("Workspace", m_localWorkspace);
  loadInstrumentAlg->setPropertyValue("InstrumentName", "PELICAN");
  loadInstrumentAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loadInstrumentAlg->executeAsChildAlg();
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadPLN::version() const { return 1; }

/// Similar algorithms. @see Algorithm::seeAlso
const std::vector<std::string> LoadPLN::seeAlso() const { return {"Load", "LoadEMU"}; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadPLN::category() const { return "DataHandling\\ANSTO"; }

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadPLN::name() const { return "LoadPLN"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadPLN::summary() const { return "Loads a PLN Hdf and linked event file into a workspace."; }

/// Return the confidence as an integer value that this algorithm can
/// load the file \p descriptor.
int LoadPLN::confidence(Kernel::NexusDescriptor &descriptor) const {
  if (descriptor.extension() != ".hdf")
    return 0;

  if (descriptor.isEntry("/entry1/site_name") && descriptor.isEntry("/entry1/instrument/fermi_chopper") &&
      descriptor.isEntry("/entry1/instrument/aperture/sh1") &&
      descriptor.isEntry("/entry1/instrument/ag1010/MEAS/Temperature") &&
      descriptor.isEntry("/entry1/instrument/detector/daq_dirname") &&
      descriptor.isEntry("/entry1/instrument/detector/dataset_number") && descriptor.isEntry("/entry1/data/hmm") &&
      descriptor.isEntry("/entry1/data/time_of_flight") && descriptor.isEntry("/entry1/data/total_counts")) {
    return 80;
  } else {
    return 0;
  }
}

/// Execute the algorithm. Establishes the filepath to the event file
/// from the HDF link and the path provided and invokes the common
// exec() function that works with the two files.
void LoadPLN::exec() {

  namespace fs = std::filesystem;

  // Open the hdf file and find the dirname and dataset number
  std::string hdfFile = getPropertyValue(FilenameStr);
  std::string evtPath = getPropertyValue(PathToBinaryStr);
  if (evtPath.empty())
    evtPath = "./";

  // if relative ./ or ../ then append to the directory for the hdf file
  if (evtPath.rfind("./") == 0 || evtPath.rfind("../") == 0) {
    fs::path hp(hdfFile);
    evtPath = fs::canonical(hp.parent_path() / evtPath).string();
  }

  // dataset index to be loaded
  m_datasetIndex = getProperty(SelectDatasetStr);

  // if path provided build the file path from the directory name and dataset
  // number from the hdf file, however if this is not a valid path then try
  // the basename with a '.bin' extension
  if (fs::is_directory(evtPath)) {
    NeXus::NXRoot root(hdfFile);
    NeXus::NXEntry entry = root.openFirstEntry();
    auto eventDir = GetNeXusValue<std::string>(entry, "instrument/detector/daq_dirname", "./", 0);
    auto dataset = GetNeXusValue<int32_t>(entry, "instrument/detector/dataset_number", 0, m_datasetIndex);
    if (dataset < 0) {
      std::string message("Negative dataset index recorded in HDF, reset to zero!");
      g_log.error(message);
      dataset = 0;
    }

    // build the path to the event file using the standard storage convention at ansto:
    //   'relpath/[daq_dirname]/DATASET_[n]/EOS.bin'
    // but if the file is missing, try relpath/{source}.bin
    std::stringstream buffer;
    buffer << eventDir.c_str() << "/DATASET_" << dataset << "/EOS.bin";
    fs::path filePath = evtPath;
    filePath /= buffer.str();
    filePath = fs::absolute(filePath);
    std::string nomPath = filePath.generic_string();
    if (fs::is_regular_file(nomPath)) {
      evtPath = nomPath;
    } else {
      fs::path hp = hdfFile;
      buffer.str("");
      buffer.clear();
      buffer << hp.stem().generic_string().c_str() << ".bin";
      fs::path path = evtPath;
      path /= buffer.str();
      path = fs::absolute(path);
      evtPath = path.generic_string();
    }
  }

  // finally check that the event file exists
  if (!fs::is_regular_file(evtPath)) {
    std::string msg = "Check path, cannot open binary event file: " + evtPath;
    throw std::runtime_error(msg);
  }

  exec(hdfFile, evtPath);
}

// register the algorithms into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadPLN)

} // namespace Mantid::DataHandling
