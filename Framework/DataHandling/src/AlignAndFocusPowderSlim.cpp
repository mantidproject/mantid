// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"

#include <H5Cpp.h>
#include <atomic>
#include <regex>

namespace Mantid::DataHandling {
using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::Workspace2D;
using Mantid::Kernel::ArrayLengthValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::EnumeratedStringProperty;
using Mantid::Kernel::TimeSeriesProperty;

namespace { // anonymous namespace
namespace PropertyNames {
const std::string FILENAME("Filename");
const std::string CAL_FILE("CalFileName");
const std::string LOAD_IDF_FROM_NXS("LoadNexusInstrumentXML");
const std::string FILTER_TIMESTART("FilterByTimeStart");
const std::string FILTER_TIMESTOP("FilterByTimeStop");
const std::string X_MIN("XMin");
const std::string X_MAX("XMax");
const std::string X_DELTA("XDelta");
const std::string BINMODE("BinningMode");
const std::string OUTPUT_WKSP("OutputWorkspace");
} // namespace PropertyNames

namespace NxsFieldNames {
const std::string TIME_OF_FLIGHT("event_time_offset");
const std::string DETID("event_id");
const std::string INDEX_ID("event_index");
} // namespace NxsFieldNames

// this is used for unit conversion to correct units
const std::string MICROSEC("microseconds");

const std::vector<std::string> binningModeNames{"Logarithmic", "Linear"};
enum class BinningMode { LOGARITHMIC, LINEAR, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;

} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AlignAndFocusPowderSlim)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string AlignAndFocusPowderSlim::name() const { return "AlignAndFocusPowderSlim"; }

/// Algorithm's version for identification. @see Algorithm::version
int AlignAndFocusPowderSlim::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AlignAndFocusPowderSlim::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AlignAndFocusPowderSlim::summary() const { return "TODO: FILL IN A SUMMARY"; }

const std::vector<std::string> AlignAndFocusPowderSlim::seeAlso() const { return {"AlignAndFocusPowderFromFiles"}; }

//----------------------------------------------------------------------------------------------
namespace { // anonymous
class NexusLoader {
public:
  NexusLoader(const bool is_time_filtered, const size_t pulse_start_index, const size_t pulse_stop_index)
      : m_is_time_filtered(is_time_filtered), m_pulse_start_index(pulse_start_index),
        m_pulse_stop_index(pulse_stop_index) {}

  static void loadPulseTimes(H5::Group &entry, std::unique_ptr<std::vector<double>> &data) {
    // /entry/DASlogs/frequency/time
    auto logs = entry.openGroup("DASlogs");        // type=NXcollection
    auto frequency = entry.openGroup("frequency"); // type=NXlog"

    auto dataset = entry.openDataSet("time");
    NeXus::H5Util::readArray1DCoerce(dataset, *data);

    // groups close themselves
  }

  void loadTOF(H5::Group &event_group, std::unique_ptr<std::vector<float>> &data,
               const std::pair<uint64_t, uint64_t> &eventRange) {
    // g_log.information(NxsFieldNames::TIME_OF_FLIGHT);
    auto tof_SDS = event_group.openDataSet(NxsFieldNames::TIME_OF_FLIGHT);

    // H5Util resizes the vector
    if (true) { // m_is_time_filtered) {
      const size_t offset = eventRange.first;
      const size_t slabsize = (eventRange.second == std::numeric_limits<size_t>::max())
                                  ? std::numeric_limits<size_t>::max()
                                  : eventRange.second - eventRange.first;
      NeXus::H5Util::readArray1DCoerce(tof_SDS, *data, slabsize, offset);
    } else {
      NeXus::H5Util::readArray1DCoerce(tof_SDS, *data);
    }

    // get the units
    std::string tof_unit;
    NeXus::H5Util::readStringAttribute(tof_SDS, "units", tof_unit);

    // Convert Tof to microseconds
    if (tof_unit != MICROSEC)
      Kernel::Units::timeConversionVector(*data, tof_unit, MICROSEC);
  }

  void loadDetid(H5::Group &event_group, std::unique_ptr<std::vector<uint32_t>> &data,
                 const std::pair<uint64_t, uint64_t> &eventRange) {
    // g_log.information(NxsFieldNames::DETID);
    auto detID_SDS = event_group.openDataSet(NxsFieldNames::DETID);

    // H5Util resizes the vector
    if (true) { // m_is_time_filtered) {
      const size_t offset = eventRange.first;
      const size_t slabsize = (eventRange.second == std::numeric_limits<size_t>::max())
                                  ? std::numeric_limits<size_t>::max()
                                  : eventRange.second - eventRange.first;
      NeXus::H5Util::readArray1DCoerce(detID_SDS, *data, slabsize, offset);
    } else {
      NeXus::H5Util::readArray1DCoerce(detID_SDS, *data);
    }
  }

private:
  void loadEventIndex(H5::Group &event_group, std::unique_ptr<std::vector<uint64_t>> &data) {
    // g_log.information(NxsFieldNames::INDEX_ID);
    auto index_SDS = event_group.openDataSet(NxsFieldNames::INDEX_ID);
    NeXus::H5Util::readArray1DCoerce(index_SDS, *data);
  }

public:
  std::pair<uint64_t, uint64_t> getEventIndexRange(H5::Group &event_group) {
    constexpr uint64_t START_DEFAULT = 0;
    constexpr uint64_t STOP_DEFAULT = std::numeric_limits<uint64_t>::max();

    if (m_is_time_filtered) {
      std::unique_ptr<std::vector<uint64_t>> event_index = std::make_unique<std::vector<uint64_t>>();
      this->loadEventIndex(event_group, event_index);

      uint64_t start_event = event_index->at(m_pulse_stop_index);
      uint64_t stop_event = STOP_DEFAULT;
      if (m_pulse_stop_index != std::numeric_limits<size_t>::max())
        stop_event = event_index->at(m_pulse_stop_index);
      return {start_event, stop_event};
    } else {
      return {START_DEFAULT, STOP_DEFAULT};
    }
  }

private:
  const bool m_is_time_filtered;
  const size_t m_pulse_start_index;
  const size_t m_pulse_stop_index;
};

class Histogrammer {
public:
  Histogrammer(const std::vector<double> *binedges, const double width, const bool linear_bins) : m_binedges(binedges) {
    m_xmin = binedges->front();
    m_xmax = binedges->back();

    if (linear_bins) {
      m_findBin = DataObjects::EventList::findLinearBin;
      m_bin_divisor = 1. / width;
      m_bin_offset = m_xmin * m_bin_divisor;
    } else {
      m_findBin = DataObjects::EventList::findLogBin;
      m_bin_divisor = 1. / log1p(abs(width)); // use this to do change of base
      m_bin_offset = log(m_xmin) * m_bin_divisor;
    }
  }

  std::optional<size_t> findBin(const double tof) const {
    // return boost::none;
    if (tof < m_xmin || tof >= m_xmax) {
      return std::nullopt;
    } else {
      return m_findBin(*m_binedges, tof, m_bin_divisor, m_bin_offset, true);
    }
  }

private:
  double m_bin_divisor;
  double m_bin_offset;
  double m_xmin;
  double m_xmax;
  const std::vector<double> *m_binedges;
  std::optional<size_t> (*m_findBin)(const MantidVec &, const double, const double, const double, const bool);
};

template <typename Type> class MinMax {
  const std::vector<Type> *vec;

public:
  Type minval;
  Type maxval;
  void operator()(const tbb::blocked_range<size_t> &range) {
    const auto [minele, maxele] = std::minmax_element(vec->cbegin() + range.begin(), vec->cbegin() + range.end());
    if (*minele < minval)
      minval = *minele;
    if (*maxele > maxval)
      maxval = *maxele;
  }

  MinMax(MinMax &other, tbb::split)
      : vec(other.vec), minval(std::numeric_limits<Type>::max()), maxval(std::numeric_limits<Type>::min()) {}

  MinMax(const std::vector<Type> *vec)
      : vec(vec), minval(std::numeric_limits<Type>::max()), maxval(std::numeric_limits<Type>::min()) {}

  void join(const MinMax &other) {
    if (other.minval < minval)
      minval = other.minval;
    if (other.maxval > maxval)
      maxval = other.maxval;
  }
};

template <typename Type> std::pair<Type, Type> parallel_minmax(const std::vector<Type> *vec) {
  constexpr size_t grainsize{2000};

  if (vec->size() < grainsize) {
    const auto [minval, maxval] = std::minmax_element(vec->cbegin(), vec->cend());
    return std::make_pair(*minval, *maxval);
  } else {
    MinMax<Type> finder(vec);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, vec->size(), grainsize), finder);
    return std::make_pair(finder.minval, finder.maxval);
  }
}

template <typename CountsType> class ProcessEventsTask {
public:
  ProcessEventsTask(const Histogrammer *histogrammer, const std::vector<uint32_t> *detids,
                    const std::vector<float> *tofs, const AlignAndFocusPowderSlim::BankCalibration *calibration,
                    std::vector<CountsType> *y_temp, const std::set<detid_t> *masked)
      : m_histogrammer(histogrammer), m_detids(detids), m_tofs(tofs), m_calibration(calibration), y_temp(y_temp),
        masked(masked) {}

  void operator()(const tbb::blocked_range<size_t> &range) const {
    for (size_t i = range.begin(); i < range.end(); ++i) {
      const auto detid = static_cast<detid_t>(m_detids->at(i));
      if (masked->contains(detid))
        continue;
      const auto tof = static_cast<double>(m_tofs->at(i)) * m_calibration->value(detid);

      const auto binnum = m_histogrammer->findBin(tof);
      if (binnum)
        y_temp->at(binnum.value())++;
    }
  }

private:
  const Histogrammer *m_histogrammer;
  const std::vector<uint32_t> *m_detids;
  const std::vector<float> *m_tofs;
  const AlignAndFocusPowderSlim::BankCalibration *m_calibration;
  std::vector<CountsType> *y_temp;
  const std::set<detid_t> *masked;
};

class ProcessBankTask {
public:
  ProcessBankTask(std::vector<std::string> &bankEntryNames, H5::H5File &h5file, const bool is_time_filtered,
                  const size_t pulse_start_index, const size_t pulse_stop_index, MatrixWorkspace_sptr &wksp,
                  const std::map<detid_t, double> &calibration, const std::set<detid_t> &masked, const double binWidth,
                  const bool linearBins)
      : m_h5file(h5file), m_bankEntries(bankEntryNames),
        m_loader(is_time_filtered, pulse_start_index, pulse_stop_index), m_wksp(wksp), m_calibration(calibration),
        m_masked(masked), m_binWidth(binWidth), m_linearBins(linearBins) {
    if (false) { // H5Freopen_async(h5file.getId(), m_h5file.getId()) < 0) {
      throw std::runtime_error("failed to reopen async");
    }
  }

  void operator()(const tbb::blocked_range<size_t> &range) const {
    // re-use vectors to save malloc/free calls
    std::unique_ptr<std::vector<uint32_t>> event_detid = std::make_unique<std::vector<uint32_t>>();
    std::unique_ptr<std::vector<float>> event_time_of_flight = std::make_unique<std::vector<float>>();

    // number of events to histogram in a single thread
    constexpr size_t GRAINSIZE_EVENT{2000};
    // number of events to read from disk at one time
    constexpr size_t EVENTS_PER_CHUNK{10000 * GRAINSIZE_EVENT};

    auto entry = m_h5file.openGroup("entry"); // type=NXentry
    for (size_t wksp_index = range.begin(); wksp_index < range.end(); ++wksp_index) {
      const auto &bankName = m_bankEntries[wksp_index];
      Kernel::Timer timer;
      std::cout << bankName << " start" << std::endl;

      // open the bank
      auto event_group = entry.openGroup(bankName); // type=NXevent_data

      // get filtering range
      auto eventRangeFull = m_loader.getEventIndexRange(event_group);
      // update range for data that is present
      if (eventRangeFull.second == std::numeric_limits<size_t>::max()) {
        auto tof_SDS = event_group.openDataSet(NxsFieldNames::TIME_OF_FLIGHT);
        const auto length_actual = static_cast<size_t>(tof_SDS.getSpace().getSelectNpoints());
        eventRangeFull.second = length_actual;
      }

      // create object so bank calibration can be re-used
      std::unique_ptr<AlignAndFocusPowderSlim::BankCalibration> calibration = nullptr;

      if (eventRangeFull.first == eventRangeFull.second) {
        // g_log.warning() << "No data for bank " << entry_name << '\n';
      } else {
        // TODO REMOVE debug print
        std::cout << bankName << " has " << eventRangeFull.second << " events\n"
                  << "   and should be read in " << (eventRangeFull.second / EVENTS_PER_CHUNK) << " chunks of "
                  << EVENTS_PER_CHUNK << "\n";

        // create a histogrammer to process the events
        auto &spectrum = m_wksp->getSpectrum(wksp_index);
        Histogrammer histogrammer(&spectrum.readX(), m_binWidth, m_linearBins);

        // std::atomic allows for multi-threaded accumulation and who cares about floats when you are just
        // counting things
        std::vector<std::atomic_uint32_t> y_temp(spectrum.dataY().size());

        // read parts of the bank at a time
        size_t event_index_start = eventRangeFull.first;
        while (event_index_start < eventRangeFull.second) {
          // H5Cpp will truncate correctly
          const std::pair<uint64_t, uint64_t> eventRangePartial{event_index_start,
                                                                event_index_start + EVENTS_PER_CHUNK};

          // load data and reuse chunk memory
          event_detid->clear();
          event_time_of_flight->clear();
          m_loader.loadTOF(event_group, event_time_of_flight, eventRangePartial);
          m_loader.loadDetid(event_group, event_detid, eventRangePartial);

          // process the events that were loaded
          const auto [minval, maxval] = parallel_minmax(event_detid.get());
          // only recreate if it doesn't already have the useful information
          if ((!calibration) || (calibration->idmin() != static_cast<detid_t>(minval)) ||
              (calibration->idmax() != static_cast<detid_t>(maxval))) {
            calibration = std::make_unique<AlignAndFocusPowderSlim::BankCalibration>(
                static_cast<detid_t>(minval), static_cast<detid_t>(maxval), m_calibration);
          }

          const auto numEvent = event_time_of_flight->size();

          // threaded processing of the events
          ProcessEventsTask task(&histogrammer, event_detid.get(), event_time_of_flight.get(), calibration.get(),
                                 &y_temp, &m_masked);
          tbb::parallel_for(tbb::blocked_range<size_t>(0, numEvent, GRAINSIZE_EVENT), task);

          event_index_start += EVENTS_PER_CHUNK;
        }
        // copy the data out into the correct spectrum
        auto &y_values = spectrum.dataY();
        std::copy(y_temp.cbegin(), y_temp.cend(), y_values.begin());

        std::cout << bankName << " stop " << timer << std::endl;
      }
    }
  }

private:
  H5::H5File m_h5file;
  const std::vector<std::string> m_bankEntries;
  mutable NexusLoader m_loader;
  MatrixWorkspace_sptr m_wksp;
  const std::map<detid_t, double> m_calibration; // detid: 1/difc
  const std::set<detid_t> m_masked;
  const double m_binWidth;
  const bool m_linearBins;
};

} // anonymous namespace

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AlignAndFocusPowderSlim::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs", "_event.nxs"};
  // docs copied/modified from LoadEventNexus
  declareProperty(std::make_unique<FileProperty>(PropertyNames::FILENAME, "", FileProperty::Load, exts),
                  "The name of the Event NeXus file to read, including its full or relative path. "
                  "The file name is typically of the form INST_####_event.nxs.");
  // this property is needed so the correct load instrument is called
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<bool>>(PropertyNames::LOAD_IDF_FROM_NXS, true, Direction::Input),
      "Reads the embedded Instrument XML from the NeXus file");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::FILTER_TIMESTART, EMPTY_DBL(),
                                                                      Direction::Input),
                  "Optional: To only include events after the provided start "
                  "time, in seconds (relative to the start of the run).");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::FILTER_TIMESTOP, EMPTY_DBL(),
                                                                      Direction::Input),
                  "Optional: To only include events before the provided stop "
                  "time, in seconds (relative to the start of the run).");
  const std::vector<std::string> cal_exts{".h5", ".hd5", ".hdf", ".cal"};
  declareProperty(std::make_unique<FileProperty>(PropertyNames::CAL_FILE, "", FileProperty::OptionalLoad, cal_exts),
                  "Optional: The .cal file containing the position correction factors. "
                  "Either this or OffsetsWorkspace needs to be specified.");
  auto positiveValidator = std::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  positiveValidator->setLower(0);
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::X_MIN, 0.25, positiveValidator,
                                                                      Direction::Input),
                  "Minimum x-value for the output binning");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::X_DELTA, 0.0016, positiveValidator,
                                                                      Direction::Input),
                  "Bin size for output data");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::X_MAX, 2.25, positiveValidator,
                                                                      Direction::Input),
                  "Minimum x-value for the output binning");
  declareProperty(std::make_unique<EnumeratedStringProperty<BinningMode, &binningModeNames>>(PropertyNames::BINMODE),
                  "Specify binning behavior ('Logorithmic')");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AlignAndFocusPowderSlim::exec() {
  // create a histogram workspace
  constexpr size_t numHist{6}; // TODO make this determined from grouping

  // These give the limits in each file as to which events we actually load (when filtering by time).
  loadStart.resize(1, 0);
  loadSize.resize(1, 0);

  // set up the output workspace binning
  const BINMODE binmode = getPropertyValue(PropertyNames::BINMODE);
  const bool linearBins = bool(binmode == BinningMode::LINEAR); // this is needed later
  const double x_delta = getProperty(PropertyNames::X_DELTA);   // this is needed later
  MatrixWorkspace_sptr wksp = createOutputWorkspace(numHist, linearBins, x_delta);

  const std::string filename = getPropertyValue(PropertyNames::FILENAME);
  const Kernel::NexusDescriptor descriptor(filename);

  const std::string ENTRY_TOP_LEVEL("entry");

  // Load the instrument
  // prog->doReport("Loading instrument"); TODO add progress bar stuff
  LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, wksp, ENTRY_TOP_LEVEL, this, &descriptor);

  const std::string cal_filename = getPropertyValue(PropertyNames::CAL_FILE);
  if (!cal_filename.empty()) {
    loadCalFile(wksp, cal_filename);
  } else {
    this->initCalibrationConstants(wksp);
  }

  /*
  // create IndexInfo
  // prog->doReport("Creating IndexInfo"); TODO add progress bar stuff
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();
  */

  // load the events
  H5::H5File h5file(filename, H5F_ACC_RDONLY);

  // filter by time
  double filter_time_start_sec = getProperty(PropertyNames::FILTER_TIMESTART);
  double filter_time_stop_sec = getProperty(PropertyNames::FILTER_TIMESTOP);

  if (filter_time_start_sec != EMPTY_DBL() || filter_time_stop_sec != EMPTY_DBL()) {
    is_time_filtered = true;
    g_log.information() << "Filtering pulses from " << filter_time_start_sec << " to " << filter_time_stop_sec << "s\n";
    std::unique_ptr<std::vector<double>> pulse_times = std::make_unique<std::vector<double>>();
    auto entry = h5file.openGroup(ENTRY_TOP_LEVEL);
    NexusLoader::loadPulseTimes(entry, pulse_times);
    g_log.information() << "Pulse times from " << pulse_times->front() << " to " << pulse_times->back()
                        << " with length " << pulse_times->size() << '\n';
    if (!std::is_sorted(pulse_times->cbegin(), pulse_times->cend())) {
      g_log.warning() << "Pulse times are not sorted, pulse time filtering will not be accurate\n";
    }

    if (filter_time_start_sec != EMPTY_DBL()) {
      const double filter_time_start = pulse_times->front() + filter_time_start_sec;
      const auto itStart = std::lower_bound(pulse_times->cbegin(), pulse_times->cend(), filter_time_start);
      if (itStart == pulse_times->cend())
        throw std::invalid_argument("Invalid pulse time filtering, start time will filter all pulses");

      pulse_start_index = std::distance(pulse_times->cbegin(), itStart);
    }

    if (filter_time_stop_sec != EMPTY_DBL()) {
      const double filter_time_stop = pulse_times->front() + filter_time_stop_sec;
      const auto itStop = std::upper_bound(pulse_times->cbegin(), pulse_times->cend(), filter_time_stop);
      if (itStop == pulse_times->cend())
        pulse_stop_index = std::numeric_limits<size_t>::max();
      else
        pulse_stop_index = std::distance(pulse_times->cbegin(), itStop);
    }

    if (pulse_start_index >= pulse_stop_index)
      throw std::invalid_argument("Invalid pulse time filtering");

    g_log.information() << "Filtering pulses from " << pulse_start_index << " to " << pulse_stop_index << '\n';
  }

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  auto itClassEntries = allEntries.find("NXevent_data");

  // temporary "map" for detid -> calibration constant

  if (itClassEntries != allEntries.end()) {
    const std::set<std::string> &classEntries = itClassEntries->second;

    // filter out the diagnostic entries
    std::vector<std::string> bankEntryNames;
    {
      const std::regex classRegex("(/entry/)([^/]*)");
      std::smatch groups;

      for (const std::string &classEntry : classEntries) {
        if (std::regex_match(classEntry, groups, classRegex)) {
          const std::string entry_name(groups[2].str());
          if (classEntry.ends_with("bank_error_events")) {
            // do nothing
          } else if (classEntry.ends_with("bank_unmapped_events")) {
            // do nothing
          } else {
            bankEntryNames.push_back(entry_name);
          }
        }
      }
    }

    // threaded processing of the banks
    ProcessBankTask task(bankEntryNames, h5file, is_time_filtered, pulse_start_index, pulse_stop_index, wksp,
                         m_calibration, m_masked, x_delta, linearBins);
    constexpr size_t GRAINSIZE_BANK{1};
    tbb::parallel_for(tbb::blocked_range<size_t>(0, bankEntryNames.size(), GRAINSIZE_BANK), task);
  }

  // close the file so child algorithms can do their thing
  h5file.close();

  // set the instrument - TODO parameters should be input information
  const double l1{43.755};
  const std::vector<double> polars{90, 90, 120, 150, 157, 65.5}; // two-theta
  const std::vector<double> azimuthals{180, 0, 0, 0, 0, 0};      // angle from positive x-axis
  const std::vector<double> l2s{2.296, 2.296, 2.070, 2.070, 2.070, 2.530};
  const std::vector<specnum_t> specids;
  wksp = editInstrumentGeometry(wksp, l1, polars, specids, l2s, azimuthals);

  // load run metadata
  // prog->doReport("Loading metadata"); TODO add progress bar stuff
  try {
    LoadEventNexus::loadEntryMetadata(filename, wksp, ENTRY_TOP_LEVEL, descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // load logs
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log"); // not used
  int nPeriods{1};
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, wksp, *this, false, nPeriods, periodLog);

  wksp->setYUnit("Counts");
  wksp->getAxis(0)->setUnit("DSpacing");
  setProperty(PropertyNames::OUTPUT_WKSP, std::move(wksp));
}

MatrixWorkspace_sptr AlignAndFocusPowderSlim::createOutputWorkspace(const size_t numHist, const bool linearBins,
                                                                    const double x_delta) {
  const double x_min = getProperty(PropertyNames::X_MIN);
  const double x_max = getProperty(PropertyNames::X_MAX);

  constexpr bool resize_xnew{true};
  constexpr bool full_bins_only{false};

  HistogramData::BinEdges XValues_new(0);
  if (linearBins) {
    const std::vector<double> params{x_min, x_delta, x_max};
    UNUSED_ARG(Kernel::VectorHelper::createAxisFromRebinParams(params, XValues_new.mutableRawData(), resize_xnew,
                                                               full_bins_only));
  } else {
    const std::vector<double> params{x_min, -1. * x_delta, x_max};
    UNUSED_ARG(Kernel::VectorHelper::createAxisFromRebinParams(params, XValues_new.mutableRawData(), resize_xnew,
                                                               full_bins_only));
  }
  MatrixWorkspace_sptr wksp = Mantid::DataObjects::create<Workspace2D>(numHist, XValues_new);
  return wksp;
}

void AlignAndFocusPowderSlim::initCalibrationConstants(API::MatrixWorkspace_sptr &wksp) {
  const auto detInfo = wksp->detectorInfo();

  for (auto iter = detInfo.cbegin(); iter != detInfo.cend(); ++iter) {
    if (!iter->isMonitor()) {
      m_calibration.emplace(iter->detid(), 1. / detInfo.difcUncalibrated(iter->index()));
    }
  }
}

void AlignAndFocusPowderSlim::loadCalFile(const Mantid::API::Workspace_sptr &inputWS, const std::string &filename) {
  auto alg = createChildAlgorithm("LoadDiffCal");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("Filename", filename);
  alg->setProperty<bool>("MakeCalWorkspace", true);
  alg->setProperty<bool>("MakeGroupingWorkspace", false);
  alg->setProperty<bool>("MakeMaskWorkspace", true);
  alg->setPropertyValue("WorkspaceName", "temp");
  alg->executeAsChildAlg();

  const ITableWorkspace_sptr calibrationWS = alg->getProperty("OutputCalWorkspace");
  for (size_t row = 0; row < calibrationWS->rowCount(); ++row) {
    const detid_t detid = calibrationWS->cell<int>(row, 0);
    const double detc = calibrationWS->cell<double>(row, 1);
    m_calibration.emplace(detid, 1. / detc);
  }

  const MaskWorkspace_sptr maskWS = alg->getProperty("OutputMaskWorkspace");
  m_masked = maskWS->getMaskedDetectors();
  g_log.debug() << "Masked detectors: " << m_masked.size() << '\n';
}

API::MatrixWorkspace_sptr AlignAndFocusPowderSlim::editInstrumentGeometry(
    API::MatrixWorkspace_sptr &wksp, const double l1, const std::vector<double> &polars,
    const std::vector<specnum_t> &specids, const std::vector<double> &l2s, const std::vector<double> &azimuthals) {
  API::IAlgorithm_sptr editAlg = createChildAlgorithm("EditInstrumentGeometry");
  editAlg->setProperty("Workspace", wksp);
  if (l1 > 0.)
    editAlg->setProperty("PrimaryFlightPath", l1);
  if (!polars.empty())
    editAlg->setProperty("Polar", polars);
  if (!specids.empty())
    editAlg->setProperty("SpectrumIDs", specids);
  if (!l2s.empty())
    editAlg->setProperty("L2", l2s);
  if (!azimuthals.empty())
    editAlg->setProperty("Azimuthal", azimuthals);
  editAlg->executeAsChildAlg();

  wksp = editAlg->getProperty("Workspace");

  return wksp;
}

// ------------------------ BankCalibration object
AlignAndFocusPowderSlim::BankCalibration::BankCalibration(const detid_t idmin, const detid_t idmax,
                                                          const std::map<detid_t, double> &calibration_map)
    : m_detid_offset(idmin) {
  // error check the id-range
  if (idmax < idmin)
    throw std::runtime_error("BAD!"); // TODO better message

  // allocate memory and set the default value to 1
  m_calibration.assign(static_cast<size_t>(idmax - idmin + 1), 1.);

  // copy over values that matter
  auto iter = calibration_map.find(idmin);
  if (iter == calibration_map.end())
    throw std::runtime_error("ALSO BAD!");
  auto iter_end = calibration_map.find(idmax);
  if (iter_end != calibration_map.end())
    ++iter_end;
  for (; iter != iter_end; ++iter) {
    const auto index = static_cast<size_t>(iter->first - m_detid_offset);
    m_calibration[index] = iter->second;
  }
}

/*
 * This assumes that everything is in range. Values that weren't in the calibration map get set to 1.
 */
const double &AlignAndFocusPowderSlim::BankCalibration::value(const detid_t detid) const {
  return m_calibration[detid - m_detid_offset];
}

const detid_t &AlignAndFocusPowderSlim::BankCalibration::idmin() const { return m_detid_offset; }
detid_t AlignAndFocusPowderSlim::BankCalibration::idmax() const {
  return m_detid_offset + static_cast<detid_t>(m_calibration.size());
}

} // namespace Mantid::DataHandling
