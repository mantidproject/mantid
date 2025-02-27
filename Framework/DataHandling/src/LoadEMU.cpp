// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadEMU.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadANSTOEventFile.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/math/special_functions/round.hpp>
#include <boost/math/tools/minima.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Util/PropertyFileConfiguration.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <utility>

namespace Mantid::DataHandling {

namespace {

// number of physical detectors
constexpr size_t HORIZONTAL_TUBES = 16;
constexpr size_t VERTICAL_TUBES = 35;
constexpr size_t DETECTOR_TUBES = HORIZONTAL_TUBES + VERTICAL_TUBES;
// analysed and direct detectors
constexpr size_t HISTO_BINS_X = DETECTOR_TUBES * 2;
constexpr size_t HISTO_BINS_Y = 1024;
constexpr size_t HISTO_BINS_Y_DENUMERATOR = 16;
constexpr size_t PIXELS_PER_TUBE = HISTO_BINS_Y / HISTO_BINS_Y_DENUMERATOR;

constexpr size_t BM_HISTOGRAMS = HISTO_BINS_X * PIXELS_PER_TUBE;
constexpr size_t HISTOGRAMS = BM_HISTOGRAMS + PIXELS_PER_TUBE;
constexpr size_t BEAM_MONITOR_BINS = 100;
constexpr size_t PSEUDO_BM_TUBE = 55;
// the half window for the running average matches the plateau width for the peak
constexpr size_t BM_HALF_WINDOW = 5;

// File loading progress boundaries
constexpr size_t Progress_LoadBinFile = 48;
constexpr size_t Progress_ReserveMemory = 4;
constexpr size_t Progress_Total = 2 * Progress_LoadBinFile + Progress_ReserveMemory;

// Algorithm parameter names
constexpr char FilenameStr[] = "Filename";
constexpr char MaskStr[] = "Mask";
constexpr char SelectDetectorTubesStr[] = "SelectDetectorTubes";
constexpr char SelectDatasetStr[] = "SelectDataset";
constexpr char OverrideDopplerFreqStr[] = "OverrideDopplerFrequency";
constexpr char OverrideDopplerPhaseStr[] = "OverrideDopplerPhase";
constexpr char FilterByTimeStartStr[] = "FilterByTimeStart";
constexpr char FilterByTimeStopStr[] = "FilterByTimeStop";
constexpr char RawDopplerTimeStr[] = "LoadAsRawDopplerTime";
constexpr char IncludePseudoBMStr[] = "IncludeBeamMonitor";
constexpr char CalibrateDopplerPhaseStr[] = "CalibrateDopplerPhase";
constexpr char PathToBinaryStr[] = "BinaryEventPath";

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
    NeXus::NXDataSetTyped<float> dataSet = entry.openNXDataSet<float>(path);
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
void MapNeXusToProperty(NeXus::NXEntry &entry, const std::string &path, const T &defval, API::LogManager &logManager,
                        const std::string &name, const T &factor, int32_t index) {

  T value = GetNeXusValue<T>(entry, path, defval, index);
  logManager.addProperty<T>(name, value * factor);
}

// sting is a special case
template <>
void MapNeXusToProperty<std::string>(NeXus::NXEntry &entry, const std::string &path, const std::string &defval,
                                     API::LogManager &logManager, const std::string &name,
                                     const std::string & /*unused*/, int32_t index) {

  std::string value = GetNeXusValue<std::string>(entry, path, defval, index);
  logManager.addProperty<std::string>(name, value);
}

template <typename T>
void MapNeXusToSeries(NeXus::NXEntry &entry, const std::string &path, const T &defval, API::LogManager &logManager,
                      const std::string &time, const std::string &name, const T &factor, int32_t index) {

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
    auto k = item.find('-');

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

// secant invert fucntion
//
template <typename F> double invert(double y, const F &f, double x0 = 0.0, const double eps = 1e-16) {
  // secant method
  double e0 = f(x0) - y;

  double x1 = x0 + eps;
  double e1 = f(x1) - y;
  int loop = 16;
  while (fabs(e0) > eps && loop-- > 0) {
    double x = (x1 * e0 - x0 * e1) / (e0 - e1);

    x1 = x0;
    e1 = e0;

    x0 = x;
    e0 = f(x0) - y;
  }

  return x0;
}

// Need to convert two different methods for direct and analysed data
// provide two distinct methods that may be called but the distances between
// the detectors is not constant so it needs to store the distance for each
// Note that observation time and TOF are in usec
//
using TofData = std::tuple<double, double>;

class ConvertTOF {
  const double m_w;
  const double m_phi;
  const double m_L0;
  const double m_v2;
  const double m_A;
  const std::vector<double> &m_L2;

  inline double L1(double t) const { return m_L0 + m_A * sin(m_w * t + m_phi); }

  inline double v1(double t) const { return m_v2 - m_A * m_w * cos(m_w * t + m_phi); }

public:
  ConvertTOF(double Amp, double freq, double phase, double L1, double v2, const std::vector<double> &L2)
      : m_w(2 * M_PI * freq), m_phi(M_PI * phase / 180.0), m_L0(L1), m_v2(v2), m_A(Amp), m_L2(L2) {}

  TofData directTOF(size_t detID, double tobs) const {

    // observation time and tof are in usec
    auto tn = [=](double t) { return t + (L1(t) + m_L2[detID]) / v1(t); };

    double tsec = tobs * 1.0e-6;
    double t0 = tsec - (m_L0 + m_L2[detID]) / m_v2;
    double tinv = invert(tsec, tn, t0);
    double tof = (m_L0 + m_L2[detID]) / v1(tinv);

    return TofData(tinv * 1.0e6, tof * 1.0e6);
  }

  TofData analysedTOF(size_t detID, double tobs) const {
    // observation time and tof are in usec
    auto tn = [=](double t) { return t + L1(t) / v1(t) + m_L2[detID] / m_v2; };

    double tsec = tobs * 1.0e-6;
    double t0 = tsec - (m_L0 + m_L2[detID]) / m_v2;
    double t = invert(tsec, tn, t0);
    double tof = m_L0 / v1(t) + m_L2[detID] / m_v2;

    return TofData(t * 1.0e6, tof * 1.0e6);
  }
};

// calculate mean of a subset of the vector
double maskedMean(const std::vector<double> &vec, const std::vector<bool> &mask) {
  if (vec.size() == 0 || vec.size() != mask.size())
    throw std::runtime_error("masked mean of empty or mismatched vectors");
  double sum = 0.0;
  size_t count = 0;
  for (size_t i = 0; i != vec.size(); i++) {
    if (!mask[i])
      continue;
    sum += vec[i];
    count++;
  }
  if (count == 0)
    throw std::runtime_error("mean of empty vector");
  return sum / static_cast<double>(count);
}

// calculate stdev for a subset of the vector
double maskedStdev(const std::vector<double> &vec, const std::vector<bool> &mask) {

  auto avg = maskedMean(vec, mask);
  size_t count = 0;
  double sum = 0.0;
  for (size_t i = 0; i != vec.size(); i++) {
    if (!mask[i])
      continue;
    sum += (vec[i] - avg) * (vec[i] - avg);
    count++;
  }
  return std::sqrt(sum / static_cast<double>(count));
}

// Calculates a running average for a given half window size assuming the data is wrapped.
// As the data are integer values, it is possible to maintain an exact sum without any
// loss of accuracy where the next filtered value is calculated by adding the leading
// point and subtracting the trailing point from the sum. As the data is wrapped the edge
// points are handled by the modulus of the array index.
//
//    |...........[.....x.....].............|
//    0           |totalWindow|             |N
//
std::vector<double> runningAverage(const std::vector<size_t> &data, size_t halfWindow) {
  const auto N = data.size();
  const size_t totalWindow = 2 * halfWindow + 1;

  // Step 1 is to calculate the sum for the first point where startIndex is the
  // first point in the total window. The start index wraps from the end of the data.
  size_t sum{0};
  size_t startIndex = N - halfWindow;
  for (size_t i = 0; i < totalWindow; i++) {
    size_t ix = (startIndex + i) % N;
    sum += data[ix];
  }

  // Save the average for the current point, drop the first point from the total window
  // sum, add the next point to the sum and shift the start index for the window.
  std::vector<double> filtered(N, 0);
  for (size_t i = 0; i < N; i++) {
    // save previous and then move to next
    filtered[i] = static_cast<double>(sum) / static_cast<double>(totalWindow);

    sum -= data[startIndex];
    sum += data[(startIndex + totalWindow) % N];
    startIndex = (startIndex + 1) % N;
  }
  return filtered;
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

  size_t size() { return _size; }

  size_t position() { return _ifs.tellg(); }

  size_t selected_position() { return _ifs.tellg(); }
};

} // anonymous namespace

namespace EMU {

// Implement emu specific handlers for the more general EMU events. The main
// differences with the current impementation of handlers:
// - tof needs to be derived from the observed time because of the doppler drive
// - emu includes direct and indirect virtual detectors that account for
//   alternate paths
// - the loader returns the doppler and auxillary time along with the
//   absolute time rather than just the primary observed time that is
//   equivalent to tof
//
// In the future the ANSTO helper and event file loader will be generalized to
// handle the instruments consistently.

class EventProcessor {
protected:
  // fields
  const std::vector<bool> &m_roi;
  const std::vector<size_t> &m_mapIndex;
  const size_t m_stride;
  const double m_framePeriod;
  const double m_gatePeriod;

  // number of frames
  size_t m_frames;
  size_t m_framesValid;

  // time boundaries
  const TimeLimits m_timeBoundary; // seconds
  const TimeLimits m_directTaux;   // microsec
  const TimeLimits m_analysedTaux; // microsec

  const bool m_includeBM;

  virtual void addEventImpl(size_t id, size_t x, size_t y, double tof) = 0;
  virtual void addPseudoBMEventImpl(size_t id, double tobs) = 0;

public:
  EventProcessor(const std::vector<bool> &roi, const std::vector<size_t> &mapIndex, const size_t stride,
                 const double framePeriod, const double gatePeriod, TimeLimits timeBoundary, TimeLimits directLimits,
                 TimeLimits analysedLimits, bool includeBM)
      : m_roi(roi), m_mapIndex(mapIndex), m_stride(stride), m_framePeriod(framePeriod), m_gatePeriod(gatePeriod),
        m_frames(0), m_framesValid(0), m_timeBoundary(std::move(timeBoundary)), m_directTaux(std::move(directLimits)),
        m_analysedTaux(std::move(analysedLimits)), m_includeBM(includeBM) {}

  void newFrame() {
    m_frames++;
    if (validFrame())
      m_framesValid++;
  }

  inline bool validFrame() const {
    double frameTime = m_framePeriod * static_cast<double>(m_frames) * 1.0e-6;
    return (frameTime >= m_timeBoundary.first && frameTime <= m_timeBoundary.second);
  }

  double duration() const {
    // test length in seconds
    return m_framePeriod * static_cast<double>(m_frames) * 1.0e-6;
  }

  inline int64_t frameStart() const {
    // returns time in nanoseconds from start of test
    auto start = m_framePeriod * static_cast<double>(m_frames);
    return static_cast<int64_t>(start * 1.0e3);
  }

  void addEvent(size_t x, size_t p, double tdop, double taux) {

    // check if in time boundaries
    if (!validFrame())
      return;

    // group pixels
    auto y = static_cast<size_t>(p / HISTO_BINS_Y_DENUMERATOR);

    // check for beam monitor tube
    if (x == PSEUDO_BM_TUBE && y < m_stride) {
      size_t id = BM_HISTOGRAMS + y;
      double ptaux = fmod(taux, m_gatePeriod);
      if (ptaux < 0)
        ptaux = ptaux + m_gatePeriod;
      addPseudoBMEventImpl(id, ptaux);
      return;
    }

    // determine detector id and check limits
    if (x >= DETECTOR_TUBES || y >= m_stride)
      return;

    // map the raw detector index to the physical model
    size_t xid = m_mapIndex[x];

    // take the modulus of the taux time to account for the
    // longer background chopper rate
    double ptaux = fmod(taux, m_gatePeriod);
    if (ptaux >= m_directTaux.first && ptaux <= m_directTaux.second)
      xid = xid + DETECTOR_TUBES;
    else if (!(ptaux >= m_analysedTaux.first && ptaux <= m_analysedTaux.second))
      return;

    size_t id = m_stride * xid + y;
    if (id >= m_roi.size())
      return;

    // check if neutron is in region of interest
    if (!m_roi[id])
      return;

    // finally pass to specific handler
    addEventImpl(id, xid, y, tdop);
  }
};

class EventCounter : public EventProcessor {
protected:
  // fields
  std::vector<size_t> &m_eventCounts;

  void addEventImpl(size_t id, size_t /*x*/, size_t /*y*/, double /*tof*/) override { m_eventCounts[id]++; }
  void addPseudoBMEventImpl(size_t id, double) override {
    if (m_includeBM) {
      m_eventCounts[id]++;
    }
  }

public:
  // construction
  EventCounter(const std::vector<bool> &roi, const std::vector<size_t> &mapIndex, const size_t stride,
               const double framePeriod, const double gatePeriod, const TimeLimits &timeBoundary,
               const TimeLimits &directLimits, const TimeLimits &analysedLimits, std::vector<size_t> &eventCounts,
               bool includeBM)
      : EventProcessor(roi, mapIndex, stride, framePeriod, gatePeriod, timeBoundary, directLimits, analysedLimits,
                       includeBM),
        m_eventCounts(eventCounts) {}

  size_t numFrames() const { return m_framesValid; }
};

class EventAssigner : public EventProcessor {
protected:
  // fields
  std::vector<EventVector_pt> &m_eventVectors;
  const ConvertTOF &m_convertTOF;
  double m_tofMin;
  double m_tofMax;
  int64_t m_startTime;
  bool m_saveAsTOF;
  const double m_binSize;
  std::vector<size_t> m_bmCounts;

  void addEventImpl(size_t id, size_t x, size_t /*y*/, double tobs) override {

    // get the absolute time for the start of the frame
    auto offset = m_startTime + frameStart();

    // convert observation time to tof and set the pulse time
    // relative to the start of the doppler cycle
    double tof = tobs;

    if (m_saveAsTOF) {
      double pulse;
      if (x < DETECTOR_TUBES)
        std::tie(pulse, tof) = m_convertTOF.analysedTOF(id, tobs);
      else
        std::tie(pulse, tof) = m_convertTOF.directTOF(id, tobs);
      offset += static_cast<int64_t>(pulse * 1e3);
    }

    if (m_tofMin > tof)
      m_tofMin = tof;
    if (m_tofMax < tof)
      m_tofMax = tof;

    auto ev = Types::Event::TofEvent(tof, Types::Core::DateAndTime(offset));
    m_eventVectors[id]->emplace_back(ev);
  }

  void addPseudoBMEventImpl(size_t id, double tobs) override {
    // get the absolute time for the start of the frame
    if (m_includeBM) {
      auto offset = m_startTime + frameStart();
      auto ev = Types::Event::TofEvent(tobs, Types::Core::DateAndTime(offset));
      m_eventVectors[id]->emplace_back(ev);
    }

    // add to the binned counts
    auto index = static_cast<size_t>(tobs / m_binSize);
    m_bmCounts[index] += 1;
  }

public:
  EventAssigner(const std::vector<bool> &roi, const std::vector<size_t> &mapIndex, const size_t stride,
                const double framePeriod, const double gatePeriod, const TimeLimits &timeBoundary,
                const TimeLimits &directLimits, const TimeLimits &analysedLimits, ConvertTOF &convert,
                std::vector<EventVector_pt> &eventVectors, int64_t startTime, bool saveAsTOF, bool includeBM)
      : EventProcessor(roi, mapIndex, stride, framePeriod, gatePeriod, timeBoundary, directLimits, analysedLimits,
                       includeBM),
        m_eventVectors(eventVectors), m_convertTOF(convert), m_tofMin(std::numeric_limits<double>::max()),
        m_tofMax(std::numeric_limits<double>::min()), m_startTime(startTime), m_saveAsTOF(saveAsTOF),
        m_binSize(gatePeriod / BEAM_MONITOR_BINS), m_bmCounts(BEAM_MONITOR_BINS, 0) {}

  double tofMin() const { return m_tofMin <= m_tofMax ? m_tofMin : 0.0; }
  double tofMax() const { return m_tofMin <= m_tofMax ? m_tofMax : 0.0; }
  const std::vector<size_t> &beamMonitorCounts() const { return m_bmCounts; }
  double binSize() const { return m_binSize; }
  size_t numBins() const { return BEAM_MONITOR_BINS; }
  size_t bmCounts() const { return std::accumulate(m_bmCounts.begin(), m_bmCounts.end(), (size_t)0); }
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

} // namespace EMU

/// Declares the properties for the two loader variants. Adds the path option
/// to the binary file and dataset set index if it is the \p hdfLoader.
template <typename FD> void LoadEMU<FD>::init(bool hdfLoader) {

  // Specify file extensions which can be associated with a specific file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  if (hdfLoader)
    exts.emplace_back(".hdf");
  else
    exts.emplace_back(".tar");
  Base::declareProperty(std::make_unique<API::FileProperty>(FilenameStr, "", API::FileProperty::Load, exts),
                        "The input filename of the stored data");

  if (hdfLoader) {
    Base::declareProperty(PathToBinaryStr, "",
                          "Relative or absolute path to the compressed binary\n"
                          "event file linked to the HDF file, eg /storage/data/");
  }

  // mask
  exts.clear();
  exts.emplace_back(".xml");
  Base::declareProperty(std::make_unique<API::FileProperty>(MaskStr, "", API::FileProperty::OptionalLoad, exts),
                        "The input filename of the mask data");

  Base::declareProperty(SelectDetectorTubesStr, "",
                        "Comma separated range of detectors tubes to be loaded,\n"
                        "  eg 16,19-45,47");

  Base::declareProperty(
      std::make_unique<API::WorkspaceProperty<API::IEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output));

  if (hdfLoader) {
    Base::declareProperty(SelectDatasetStr, 0, "Select the index for the dataset to be loaded.");
  }

  Base::declareProperty(OverrideDopplerFreqStr, EMPTY_DBL(), "Override the Doppler frequency, in Hertz.");

  Base::declareProperty(OverrideDopplerPhaseStr, EMPTY_DBL(), "Override the Doppler phase, in degrees.");

  Base::declareProperty(CalibrateDopplerPhaseStr, false,
                        "Calibrate the Doppler phase prior to TOF conversion,\n"
                        "ignored if imported as Doppler time or phase entered");

  Base::declareProperty(RawDopplerTimeStr, false,
                        "Import file as observed time relative the Doppler\n"
                        "drive, in microsecs.");

  Base::declareProperty(IncludePseudoBMStr, false, "Include the individual beam monitor events as spectra.");

  Base::declareProperty(FilterByTimeStartStr, 0.0,
                        "Only include events after the provided start time, in "
                        "seconds (relative to the start of the run).");

  Base::declareProperty(FilterByTimeStopStr, EMPTY_DBL(),
                        "Only include events before the provided stop time, in "
                        "seconds (relative to the start of the run).");

  std::string grpOptional = "Filters";
  Base::setPropertyGroup(FilterByTimeStartStr, grpOptional);
  Base::setPropertyGroup(FilterByTimeStopStr, grpOptional);
}

/// Creates an event workspace and sets the \p title.
template <typename FD> void LoadEMU<FD>::createWorkspace(const std::string &title) {

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
///   Reposition the relevant neutronic values for model based on the parameters
///   Load the data values and convert to TOF
///   Setting up the masks
template <typename FD> void LoadEMU<FD>::exec(const std::string &hdfFile, const std::string &eventFile) {

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
  // ----------------------------------------
  logManager.addProperty(SelectDatasetStr, m_datasetIndex);
  loadParameters(hdfFile, logManager);
  prog.doReport("creating instrument");
  loadInstrument();

  // Get the region of interest and filters and save to log
  //
  std::string maskfile = Base::getPropertyValue(MaskStr);
  std::string seltubes = Base::getPropertyValue(SelectDetectorTubesStr);
  logManager.addProperty(SelectDetectorTubesStr, seltubes);
  logManager.addProperty(MaskStr, maskfile);

  std::vector<bool> roi = createRoiVector(seltubes, maskfile);
  double timeMaxBoundary = Base::getProperty(FilterByTimeStopStr);
  if (Base::isEmpty(timeMaxBoundary))
    timeMaxBoundary = std::numeric_limits<double>::infinity();
  TimeLimits timeBoundary(Base::getProperty(FilterByTimeStartStr), timeMaxBoundary);

  // lambda to simplify loading instrument parameters
  auto instr = m_localWorkspace->getInstrument();
  auto iparam = [&instr](const std::string &tag) { return instr->getNumberParameter(tag)[0]; };

  // Update the neutronic positions for the indirect detectors
  // noting that the vertical and horizontal are a continuous
  // sequence starting from 0
  //
  double sampleAnalyser = iparam("SampleAnalyser");
  auto endID = static_cast<detid_t>(DETECTOR_TUBES * PIXELS_PER_TUBE);
  for (detid_t detID = 0; detID < endID; ++detID)
    updateNeutronicPostions(detID, sampleAnalyser);

  // get the detector map from raw input to a physical detector
  //
  std::string dmapStr = instr->getParameterAsString("DetectorMap");
  std::vector<size_t> detMapIndex = std::vector<size_t>(DETECTOR_TUBES, 0);
  mapRangeToIndex(dmapStr, detMapIndex, [](size_t n) { return n; });

  // Collect the L2 distances, Doppler characteristics and
  // initiate the TOF converter
  //
  loadDetectorL2Values();
  loadDopplerParameters(logManager);
  double v2 = iparam("AnalysedV2");           // analysed velocity in metres per sec
  double framePeriod = 1.0e6 / m_dopplerFreq; // period and max direct as microsec
  double sourceSample = iparam("SourceSample");
  ConvertTOF convertTOF(m_dopplerAmpl * m_dopplerRun, m_dopplerFreq, m_dopplerPhase, sourceSample, v2, m_detectorL2);

  // Load the events file
  // --------------------
  // First count the number of events to reserve memory and then assign the
  // events to the detectors

  // load events
  size_t numberHistograms = m_localWorkspace->getNumberHistograms();
  std::vector<EventVector_pt> eventVectors(numberHistograms, nullptr);
  std::vector<size_t> eventCounts(numberHistograms, 0);

  // Discriminating between direct and analysed is based on the auxillary time
  // and is determined by the graphite chopper frequency and v2 which are stable
  // so these limits are kept in the instrument parameter file. Convert from
  // milsec to microsec.
  TimeLimits directLimits(1000.0 * iparam("DirectTauxMin"), 1000.0 * iparam("DirectTauxMax"));
  TimeLimits analysedLimits(1000.0 * iparam("AnalysedTauxMin"), 1000.0 * iparam("AnalysedTauxMax"));

  // fabs because the value can be negative
  double gatePeriod = 1.0e6 / fabs(logManager.getTimeSeriesProperty<double>("GraphiteChopperFrequency")->firstValue());

  // count total events per pixel and reserve necessary memory
  bool includeBM = Base::getProperty(IncludePseudoBMStr);
  EMU::EventCounter eventCounter(roi, detMapIndex, PIXELS_PER_TUBE, framePeriod, gatePeriod, timeBoundary, directLimits,
                                 analysedLimits, eventCounts, includeBM);
  EMU::loadEvents(prog, "loading neutron counts", eventFile, eventCounter);
  ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists", numberHistograms, Progress_ReserveMemory);
  prepareEventStorage(progTracker, eventCounts, eventVectors);

  // now perform the actual event collection and TOF convert if necessary
  // if a phase calibration is required then load it as raw doppler time
  // perform the calibration and then convert to TOF
  Types::Core::DateAndTime startTime(m_startRun);
  auto start_nanosec = startTime.totalNanoseconds();
  bool saveAsTOF = !Base::getProperty(RawDopplerTimeStr);
  bool loadAsTOF = !m_calibrateDoppler && saveAsTOF;
  EMU::EventAssigner eventAssigner(roi, detMapIndex, PIXELS_PER_TUBE, framePeriod, gatePeriod, timeBoundary,
                                   directLimits, analysedLimits, convertTOF, eventVectors, start_nanosec, loadAsTOF,
                                   includeBM);
  EMU::loadEvents(prog, "loading neutron events (TOF)", eventFile, eventAssigner);

  // determine the minimum and maximum beam rate per sec
  auto filteredBM = runningAverage(eventAssigner.beamMonitorCounts(), BM_HALF_WINDOW);
  auto res = std::minmax_element(filteredBM.begin(), filteredBM.end());
  auto ratePerSec = static_cast<double>(eventAssigner.numBins()) / eventCounter.duration();
  auto minBM = *res.first * ratePerSec;
  auto maxBM = *res.second * ratePerSec;
  AddSinglePointTimeSeriesProperty<double>(logManager, m_startRun, "BeamMonitorBkgRate", minBM);
  AddSinglePointTimeSeriesProperty<double>(logManager, m_startRun, "BeamMonitorRate", maxBM);
  AddSinglePointTimeSeriesProperty<int>(logManager, m_startRun, "MonitorCounts",
                                        static_cast<int>(eventAssigner.bmCounts()));

  // perform a calibration and then TOF conversion if necessary
  // and update the tof limits
  auto minTOF = eventAssigner.tofMin();
  auto maxTOF = eventAssigner.tofMax();
  if (m_calibrateDoppler) {
    calibrateDopplerPhase(eventCounts, eventVectors);
    if (saveAsTOF) {
      dopplerTimeToTOF(eventVectors, minTOF, maxTOF);
    }
  }
  AddSinglePointTimeSeriesProperty<double>(logManager, m_startRun, "DopplerPhase", m_dopplerPhase);

  // just to make sure the bins hold it all and setup the detector masks
  m_localWorkspace->setAllX(HistogramData::BinEdges{std::max(0.0, floor(minTOF)), maxTOF + 1});
  setupDetectorMasks(roi);

  // set log values
  auto frame_count = eventCounter.numFrames();
  AddSinglePointTimeSeriesProperty<int>(logManager, m_startRun, "frame_count", static_cast<int>(frame_count));

  // add the scan period in secs to the log
  auto scan_period = static_cast<double>(frame_count + 1) / m_dopplerFreq;
  AddSinglePointTimeSeriesProperty<double>(logManager, m_startRun, "ScanPeriod", scan_period);

  std::string filename = Base::getPropertyValue(FilenameStr);
  logManager.addProperty("filename", filename);

  Types::Core::time_duration duration =
      boost::posix_time::microseconds(static_cast<boost::int64_t>(eventCounter.duration() * 1.0e6));
  Types::Core::DateAndTime endTime(startTime + duration);
  logManager.addProperty("start_time", startTime.toISO8601String());
  logManager.addProperty("end_time", endTime.toISO8601String());
  logManager.addProperty<double>("dur", eventCounter.duration());

  // Finally add the time-series parameter explicitly
  loadEnvironParameters(hdfFile, logManager);

  Base::setProperty("OutputWorkspace", m_localWorkspace);
}

/// Set up the detector masks to the region of interest \p roi.
template <typename FD> void LoadEMU<FD>::setupDetectorMasks(const std::vector<bool> &roi) {

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

    auto maskingAlg = Base::createChildAlgorithm("MaskDetectors");
    maskingAlg->setProperty("Workspace", m_localWorkspace);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }
}

/// Allocate space for the event storage in \p eventVectors after the
/// \p eventCounts have been determined.
template <typename FD>
void LoadEMU<FD>::prepareEventStorage(ANSTO::ProgressTracker &progTracker, const std::vector<size_t> &eventCounts,
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

/// Get the Doppler parameters and record to the log manager, \p logm.
template <typename FD> void LoadEMU<FD>::loadDopplerParameters(API::LogManager &logm) {

  auto instr = m_localWorkspace->getInstrument();

  // use nominal frequency based on amp and velocity unless overridden
  m_dopplerAmpl = logm.getTimeSeriesProperty<double>("DopplerAmplitude")->firstValue();
  m_dopplerRun = logm.getTimeSeriesProperty<int32_t>("DopplerRun")->firstValue();
  m_dopplerFreq = Base::getProperty(OverrideDopplerFreqStr);
  if (Base::isEmpty(m_dopplerFreq)) {
    auto doppVel = logm.getTimeSeriesProperty<double>("DopplerVelocity")->firstValue();
    m_dopplerFreq = 0.5 * doppVel / (M_PI * m_dopplerAmpl);
  }
  AddSinglePointTimeSeriesProperty<double>(logm, m_startRun, "DopplerFrequency", m_dopplerFreq);

  m_dopplerPhase = Base::getProperty(OverrideDopplerPhaseStr);
  m_calibrateDoppler = Base::getProperty(CalibrateDopplerPhaseStr) && Base::isEmpty(m_dopplerPhase);
  if (Base::isEmpty(m_dopplerPhase)) {
    // sinusoidal motion crossing a threshold with a delay
    double doppThreshold = instr->getNumberParameter("DopplerReferenceThreshold")[0];
    double doppDelay = instr->getNumberParameter("DopplerReferenceDelay")[0];
    m_dopplerPhase = 180.0 - asin(0.001 * doppThreshold / m_dopplerAmpl) * 180.0 / M_PI + doppDelay * m_dopplerFreq;
  }

  // problem adding 'bool' to log
  int32_t calPhase = (m_calibrateDoppler ? 1 : 0);
  logm.addProperty("CalibratePhase", calPhase);
}

/// Calibrate the doppler phase based on the analysed events using
/// the \p eventCounts and \p eventVectors.
template <typename FD>
void LoadEMU<FD>::calibrateDopplerPhase(const std::vector<size_t> &eventCounts,
                                        const std::vector<EventVector_pt> &eventVectors) {

  // get the doppler parameters
  auto instr = m_localWorkspace->getInstrument();
  double v2 = instr->getNumberParameter("AnalysedV2")[0];
  double l1 = instr->getNumberParameter("SourceSample")[0];

  // get the number of analysed events and initial doppler time
  auto startID = static_cast<size_t>(HORIZONTAL_TUBES * PIXELS_PER_TUBE);
  auto endID = static_cast<size_t>(DETECTOR_TUBES * PIXELS_PER_TUBE);
  size_t numEvents = 0;
  for (size_t i = startID; i < endID; i++)
    numEvents += eventCounts[i];
  if (numEvents == 0)
    throw std::runtime_error("no analysed events for phase calibration");
  std::vector<double> nVel(numEvents);
  std::vector<size_t> nMap(numEvents);
  std::vector<bool> nCnd(numEvents);
  constexpr size_t NHIST = 100;
  std::vector<int> histogram(NHIST + 1, 0);

  // define the cost function to optimize phase
  auto costFn = [&, this](double phase) {
    ConvertTOF convTOF(m_dopplerAmpl * m_dopplerRun, m_dopplerFreq, phase, l1, v2, m_detectorL2);

    // convert each analysed event to source velocity
    size_t ix = 0;
    double tof, pulse;
    for (size_t i = startID; i < endID; i++) {
      for (auto const &x : *eventVectors[i]) {
        std::tie(pulse, tof) = convTOF.analysedTOF(i, x.tof());
        auto tof1 = 1e-6 * tof - m_detectorL2[i] / v2;
        nVel[ix++] = l1 / tof1;
      }
    }

    // now histogram the data and create the map from velocity to hist
    auto ixlim = std::minmax_element(nVel.begin(), nVel.end());
    auto vmin = nVel[ixlim.first - nVel.begin()];
    auto vmax = nVel[ixlim.second - nVel.begin()];
    int maxHist = 0;
    std::fill(histogram.begin(), histogram.end(), 0);
    auto delta = (vmax - vmin) / NHIST;
    for (size_t i = 0; i < numEvents; i++) {
      auto v = nVel[i];
      auto j = static_cast<size_t>(std::floor((v - vmin) / delta));
      histogram[j]++;
      if (histogram[j] > maxHist)
        maxHist = histogram[j];
      nMap[i] = j;
    }

    // determine the points above the 25% threshold
    auto minLevel = static_cast<int>(maxHist / 4);
    for (size_t i = 0; i < numEvents; i++) {
      nCnd[i] = (histogram[nMap[i]] >= minLevel ? true : false);
    }

    // calculate the standard deviation for the points above the threshold
    auto cost = maskedStdev(nVel, nCnd);
    return cost;
  };

  // call the optimizer and update the doppler phase value
  // limit the optimization to 30 iterations
  int bits = std::numeric_limits<double>::digits;
  boost::uintmax_t itn = 30;
  using boost::math::tools::brent_find_minima;
  auto minPhase = m_dopplerPhase - 5.0;
  auto maxPhase = m_dopplerPhase + 5.0;
  auto r = brent_find_minima(costFn, minPhase, maxPhase, bits, itn);
  m_dopplerPhase = r.first;
}

/// Convert the doppler time to TOF for all the events in \p eventVectors and
/// time of flight range as \p minTOF and \p maxTOF.
template <typename FD>
void LoadEMU<FD>::dopplerTimeToTOF(std::vector<EventVector_pt> &eventVectors, double &minTOF, double &maxTOF) {

  // get the doppler parameters and initialise TOD converter
  auto instr = m_localWorkspace->getInstrument();
  double v2 = instr->getNumberParameter("AnalysedV2")[0];
  double l1 = instr->getNumberParameter("SourceSample")[0];
  ConvertTOF convTOF(m_dopplerAmpl * m_dopplerRun, m_dopplerFreq, m_dopplerPhase, l1, v2, m_detectorL2);

  // run through all the events noting that analysed event are in
  // the bottom half of the detector ids
  auto start = true;
  auto directID = static_cast<size_t>(DETECTOR_TUBES * PIXELS_PER_TUBE);
  for (size_t id = 0; id < eventVectors.size(); id++) {
    for (auto &x : *eventVectors[id]) {
      double tof, pulse;
      if (id < directID)
        std::tie(pulse, tof) = convTOF.analysedTOF(id, x.tof());
      else
        std::tie(pulse, tof) = convTOF.directTOF(id, x.tof());

      // update the pulse time and tof
      int64_t pulseTime = x.pulseTime().totalNanoseconds();
      pulseTime += static_cast<int64_t>(pulse * 1000);
      x = Types::Event::TofEvent(tof, Types::Core::DateAndTime(pulseTime));

      if (start) {
        minTOF = maxTOF = x.tof();
        start = false;
      } else {
        minTOF = std::min(minTOF, x.tof());
        maxTOF = std::max(maxTOF, x.tof());
      }
    }
  }
}

/// Recovers the L2 neutronic distance for each detector.
template <typename FD> void LoadEMU<FD>::loadDetectorL2Values() {

  m_detectorL2 = std::vector<double>(HISTOGRAMS);
  const auto &detectorInfo = m_localWorkspace->detectorInfo();
  auto detIDs = detectorInfo.detectorIDs();
  for (const auto detID : detIDs) {
    auto ix = detectorInfo.indexOf(detID);
    double l2 = detectorInfo.l2(ix);
    m_detectorL2[detID] = l2;
  }
}

/// Update the neutronic position for the \p detID using the distance
/// from the source and the sample to analyser distance, \p sampleAnalyser.
template <typename FD> void LoadEMU<FD>::updateNeutronicPostions(detid_t detID, double sampleAnalyser) {

  Geometry::Instrument_const_sptr instrument = m_localWorkspace->getInstrument();
  auto &compInfo = m_localWorkspace->mutableComponentInfo();

  try {
    auto component = instrument->getDetector(detID);
    double rho, theta, phi;
    V3D position = component->getPos();
    position.getSpherical(rho, theta, phi);

    double scale = -(2 * sampleAnalyser + rho) / rho;
    position *= scale;

    const auto componentIndex = compInfo.indexOf(component->getComponentID());
    compInfo.setPosition(componentIndex, position);
  } catch (const std::runtime_error &) {
    // just continue with the remainder
  }
}

/// Region of interest is defined by the \p selected detectors and the
/// \p maskfile.
template <typename FD>
std::vector<bool> LoadEMU<FD>::createRoiVector(const std::string &selected, const std::string &maskfile) {

  std::vector<bool> result(HISTOGRAMS, true);

  // turn off pixels linked to missing tubes
  if (!selected.empty()) {
    std::vector<bool> tubes(HISTO_BINS_X, false);
    mapRangeToIndex(selected, tubes, [](size_t) { return true; });
    for (size_t i = 0; i < HISTO_BINS_X; i++) {
      if (tubes[i] == false) {
        for (size_t j = 0; j < PIXELS_PER_TUBE; j++) {
          result[i * PIXELS_PER_TUBE + j] = false;
        }
      }
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
template <typename FD> void LoadEMU<FD>::loadParameters(const std::string &hdfFile, API::LogManager &logm) {

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
        boost::posix_time::microseconds(static_cast<boost::int64_t>((nthTime - baseTime) * 1.0e6));
    Types::Core::DateAndTime startDataset(startTime + duration);
    m_startRun = startDataset.toISO8601String();
  } else {
    m_startRun = startTime.toISO8601String();
  }

  MapNeXusToSeries<double>(entry, "instrument/doppler/ctrl/amplitude", 75.0, logm, m_startRun, "DopplerAmplitude",
                           0.001, m_datasetIndex);
  MapNeXusToSeries<double>(entry, "instrument/doppler/ctrl/velocity", 4.7, logm, m_startRun, "DopplerVelocity", 1,
                           m_datasetIndex);
  MapNeXusToSeries<int>(entry, "instrument/doppler/ctrl/run_cmd", 1, logm, m_startRun, "DopplerRun", 1, m_datasetIndex);

  MapNeXusToSeries<double>(entry, "instrument/chpr/background/actspeed", 1272.8, logm, m_startRun,
                           "BackgroundChopperFrequency", 1.0 / 60, 0);
  MapNeXusToSeries<double>(entry, "instrument/chpr/graphite/actspeed", 2545.6, logm, m_startRun,
                           "GraphiteChopperFrequency", 1.0 / 60, 0);
  // hz tube gap or equivalent to be added later - reverts to default
  MapNeXusToSeries<double>(entry, "instrument/hztubegap", 0.02, logm, m_startRun, "horizontal_tubes_gap", 1.0, 0);
  // add the reactor power to the log
  MapNeXusToSeries<double>(entry, "instrument/source/power", 20.0, logm, m_startRun, "ReactorPower", 1.0,
                           m_datasetIndex);
  // fix for source position when loading IDF
  MapNeXusToProperty<double>(entry, "instrument/doppler/tosource", 2.035, logm, "SourceSample", 1.0, 0);
}

/// Load the environment variables from the \p hdfFile and save as
/// time series to the log manager, \p logm.
template <typename FD> void LoadEMU<FD>::loadEnvironParameters(const std::string &hdfFile, API::LogManager &logm) {

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
template <typename FD> void LoadEMU<FD>::loadInstrument() {

  // loads the IDF and parameter file
  auto loadInstrumentAlg = Base::createChildAlgorithm("LoadInstrument");
  loadInstrumentAlg->setProperty("Workspace", m_localWorkspace);
  loadInstrumentAlg->setPropertyValue("InstrumentName", "EMUau");
  loadInstrumentAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loadInstrumentAlg->executeAsChildAlg();
}

//--------- explicit template instantiation -----------

// Instantiate base class for LoadEMU's
template class LoadEMU<Kernel::FileDescriptor>;
template class LoadEMU<Kernel::NexusHDF5Descriptor>;

// -------- EMU Hdf loader -----------------------

/// Algorithm's version for identification. @see Algorithm::version
int LoadEMUHdf::version() const { return 1; }

/// Similar algorithms. @see Algorithm::seeAlso
const std::vector<std::string> LoadEMUHdf::seeAlso() const { return {"Load", "LoadQKK"}; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadEMUHdf::category() const { return "DataHandling\\ANSTO"; }

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadEMUHdf::name() const { return "LoadEMUHdf"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadEMUHdf::summary() const { return "Loads an EMU Hdf and linked event file into a workspace."; }

/// Return the confidence as an integer value that this algorithm can
/// load the file \p descriptor.
int LoadEMUHdf::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  if (descriptor.extension() != ".hdf")
    return 0;

  if (descriptor.isEntry("/entry1/site_name") && descriptor.isEntry("/entry1/instrument/doppler/ctrl/velocity") &&
      descriptor.isEntry("/entry1/instrument/doppler/ctrl/amplitude") &&
      descriptor.isEntry("/entry1/instrument/detector/daq_dirname") &&
      descriptor.isEntry("/entry1/instrument/detector/dataset_number") &&
      descriptor.isEntry("/entry1/data/hmm_total_t_ds0") && descriptor.isEntry("/entry1/data/hmm_total_t_ds1") &&
      descriptor.isEntry("/entry1/data/hmm_total_xt_ds0") && descriptor.isEntry("/entry1/data/hmm_total_xt_ds1")) {
    return 80;
  } else {
    return 0;
  }
}

/// Initialise the algorithm and declare the properties for the
/// nexus descriptor.
void LoadEMUHdf::init() { LoadEMU<Kernel::NexusHDF5Descriptor>::init(true); }

/// Execute the algorithm. Establishes the filepath to the event file
/// from the HDF link and the path provided and invokes the common
// exec() function that works with the two files.
void LoadEMUHdf::exec() {

  namespace fs = std::filesystem;

  // Open the hdf file and find the dirname and dataset number
  std::string hdfFile = Base::getPropertyValue(FilenameStr);
  std::string evtPath = Base::getPropertyValue(PathToBinaryStr);
  if (evtPath.empty())
    evtPath = "./";

  // if relative ./ or ../ then append to the directory for the hdf file
  if (evtPath.rfind("./") == 0 || evtPath.rfind("../") == 0) {
    fs::path hp(hdfFile);
    evtPath = fs::canonical(hp.parent_path() / evtPath).string();
  }

  // dataset index to be loaded
  m_datasetIndex = Base::getProperty(SelectDatasetStr);

  // if path provided build the file path from the directory name and dataset
  // number from the hdf file, however if this is not a valid path then try
  // the basename with a '.bin' extension
  if (fs::is_directory(evtPath)) {
    NeXus::NXRoot root(hdfFile);
    NeXus::NXEntry entry = root.openFirstEntry();
    auto eventDir = GetNeXusValue<std::string>(entry, "instrument/detector/daq_dirname", "./", 0);
    auto dataset = GetNeXusValue<int32_t>(entry, "instrument/detector/dataset_number", 0, m_datasetIndex);
    if (dataset < 0) {
      g_log.warning("Negative dataset index recorded in HDF, reset to zero!");
      dataset = 0;
    }

    // build the path to the event file using the standard storage convention at ansto:
    //   'relpath/[daq_dirname]/DATASET_[n]/EOS.bin'
    // but if the file is missing, try relpath/{source}.bin
    fs::path filePath = fs::absolute(fs::path(evtPath) / eventDir / ("DATASET_" + std::to_string(dataset)) / "EOS.bin");
    if (!fs::is_regular_file(filePath)) {
      filePath = fs::absolute(fs::path(hdfFile).replace_extension(".bin"));
    }
    evtPath = filePath.generic_string();
  }

  // finally check that the event file exists
  if (!fs::is_regular_file(evtPath)) {
    std::string msg = "Check path, cannot open binary event file: " + evtPath;
    throw std::runtime_error(msg);
  }

  LoadEMU<Kernel::NexusHDF5Descriptor>::exec(hdfFile, evtPath);
}

// -------- EMU Tar loader -----------------------

/// Algorithm's version for identification. @see Algorithm::version
int LoadEMUTar::version() const { return 1; }

/// Similar algorithms. @see Algorithm::seeAlso
const std::vector<std::string> LoadEMUTar::seeAlso() const { return {"Load", "LoadQKK"}; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadEMUTar::category() const { return "DataHandling\\ANSTO"; }

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadEMUTar::name() const { return "LoadEMU"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadEMUTar::summary() const {
  return "Loads an EMU tar file, containing the Hdf and event file, into a "
         "workspace.";
}

/// Return the confidence as an integer value that this algorithm can
/// load the file \p descriptor.
int LoadEMUTar::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension() != ".tar")
    return 0;

  ANSTO::Tar::File file(descriptor.filename());
  if (!file.good())
    return 0;

  size_t hdfFiles = 0;
  size_t binFiles = 0;
  const std::vector<std::string> &subFiles = file.files();
  for (const auto &subFile : subFiles) {
    auto len = subFile.length();
    if ((len > 4) && (subFile.find_first_of("\\/", 0, 2) == std::string::npos)) {
      if ((subFile.rfind(".hdf") == len - 4) && (subFile.compare(0, 3, "EMU") == 0))
        hdfFiles++;
      else if (subFile.rfind(".bin") == len - 4)
        binFiles++;
    }
  }

  return (hdfFiles == 1) && (binFiles == 1) ? 50 : 0;
}

/// Initialise the algorithm and declare the standard properties for the
/// general file descriptor.
void LoadEMUTar::init() { LoadEMU<Kernel::FileDescriptor>::init(false); }

/// Execute the algorithm. Extracts the hdf and event file from the tar
/// and invokes the invokes the common exec() function that works with
/// the two files.
void LoadEMUTar::exec() {

  // Opens the tar and extracts the hdf and event data to temporary files
  std::string filename = Base::getPropertyValue(FilenameStr);
  ANSTO::Tar::File tarFile(filename);
  if (!tarFile.good())
    throw std::invalid_argument("invalid EMU tar file");

  // dataset selection not supported in tar version - order is not guaranteed
  m_datasetIndex = 0;

  // lambda functions to find the first file of extension and to extract
  // the file
  const std::vector<std::string> &files = tarFile.files();
  auto selectFile = [&](const std::string &ext) {
    auto itf = std::find_if(files.cbegin(), files.cend(),
                            [&ext](const std::string &file) { return file.rfind(ext) == file.length() - 4; });
    if (itf == files.end())
      throw std::runtime_error("missing tar file data");
    else
      tarFile.select(itf->c_str());
  };
  auto extractFile = [&](Poco::TemporaryFile &tfile) {
    std::shared_ptr<FILE> handle(fopen(tfile.path().c_str(), "wb"), fclose);
    if (handle) {
      // copy content
      char buffer[4096];
      size_t bytesRead;
      while (0 != (bytesRead = tarFile.read(buffer, sizeof(buffer))))
        fwrite(buffer, bytesRead, 1, handle.get());
      handle.reset();
    }
  };

  // extract hdf file into tmp file
  selectFile(".hdf");
  Poco::TemporaryFile hdfFile;
  extractFile(hdfFile);

  // extract the event file
  selectFile(".bin");
  Poco::TemporaryFile eventFile;
  extractFile(eventFile);

  // call the common loader
  LoadEMU<Kernel::FileDescriptor>::exec(hdfFile.path(), eventFile.path());
}

// register the algorithms into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadEMUTar)
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadEMUHdf)

} // namespace Mantid::DataHandling
