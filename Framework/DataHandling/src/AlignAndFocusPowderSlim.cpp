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
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
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
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::EnumeratedStringProperty;
using Mantid::Kernel::TimeSeriesProperty;

namespace { // anonymous namespace
namespace PropertyNames {
const std::string FILENAME("Filename");
const std::string CAL_FILE("CalFileName");
const std::string FILTER_TIMESTART("FilterByTimeStart");
const std::string FILTER_TIMESTOP("FilterByTimeStop");
const std::string X_MIN("XMin");
const std::string X_MAX("XMax");
const std::string X_DELTA("XDelta");
const std::string BINMODE("BinningMode");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string READ_SIZE_FROM_DISK("ReadSizeFromDisk");
const std::string EVENTS_PER_THREAD("EventsPerThread");
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

const size_t NUM_HIST{6}; // TODO make this determined from groupin

// TODO refactor this to use the actual grouping
double getFocussedPostion(const detid_t detid, const std::vector<double> &difc_focus) {
  // grouping taken from the IDF for VULCAN
  if (detid < 0) {
    throw std::runtime_error("detid < 0 is not supported");
  } else if (detid < 100000) { // bank1 0-99095
    return difc_focus[0];
  } else if (detid < 200000) { // bank2 100000-199095
    return difc_focus[1];
  } else if (detid < 300000) { // bank3 200000-289095
    return difc_focus[2];
  } else if (detid < 400000) { // bank4 300000-389095
    return difc_focus[3];
  } else if (detid < 500000) { // bank5 400000-440095
    return difc_focus[4];
  } else if (detid < 600000) { // bank6 500000-554095
    return difc_focus[5];
  } else {
    throw std::runtime_error("detid > 600000 is not supported");
  }
}

} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AlignAndFocusPowderSlim)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string AlignAndFocusPowderSlim::name() const { return "AlignAndFocusPowderSlim"; }

/// Algorithm's version for identification. @see Algorithm::version
int AlignAndFocusPowderSlim::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AlignAndFocusPowderSlim::category() const { return "Workflow\\Diffraction"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AlignAndFocusPowderSlim::summary() const {
  return "VULCAN ONLY Algorithm to focus powder diffraction data into a number of histograms according to a grouping "
         "scheme defined in a CalFile.";
}

const std::vector<std::string> AlignAndFocusPowderSlim::seeAlso() const { return {"AlignAndFocusPowderFromFiles"}; }

//----------------------------------------------------------------------------------------------
namespace { // anonymous
std::vector<double> calculate_difc_focused(const double l1, const std::vector<double> &l2s,
                                           const std::vector<double> &polars) {
  constexpr double deg2rad = M_PI / 180.;

  std::vector<double> difc;

  std::transform(l2s.cbegin(), l2s.cend(), polars.cbegin(), std::back_inserter(difc),
                 [l1, deg2rad](const auto &l2, const auto &polar) {
                   return 1. / Kernel::Units::tofToDSpacingFactor(l1, l2, deg2rad * polar, 0.);
                 });

  return difc;
}

class NexusLoader {
public:
  NexusLoader(const bool is_time_filtered, const size_t pulse_start_index, const size_t pulse_stop_index)
      : m_is_time_filtered(is_time_filtered), m_pulse_start_index(pulse_start_index),
        m_pulse_stop_index(pulse_stop_index) {}

  static void loadPulseTimes(H5::Group &entry, std::unique_ptr<std::vector<double>> &data) {
    // /entry/DASlogs/frequency/time
    auto logs = entry.openGroup("DASlogs");       // type=NXcollection
    auto frequency = logs.openGroup("frequency"); // type=NXlog"

    auto dataset = frequency.openDataSet("time");
    NeXus::H5Util::readArray1DCoerce(dataset, *data); // pass by reference

    // groups close themselves
  }

  void loadTOF(H5::Group &event_group, std::unique_ptr<std::vector<float>> &data,
               const std::pair<uint64_t, uint64_t> &eventRange) {
    // g_log.information(NxsFieldNames::TIME_OF_FLIGHT);
    auto tof_SDS = event_group.openDataSet(NxsFieldNames::TIME_OF_FLIGHT);

    // H5Util resizes the vector
    const size_t offset = eventRange.first;
    const size_t slabsize = (eventRange.second == std::numeric_limits<size_t>::max())
                                ? std::numeric_limits<size_t>::max()
                                : eventRange.second - eventRange.first;
    NeXus::H5Util::readArray1DCoerce(tof_SDS, *data, slabsize, offset);

    // get the units
    std::string tof_unit;
    NeXus::H5Util::readStringAttribute(tof_SDS, "units", tof_unit);

    // Convert Tof to microseconds
    if (tof_unit != MICROSEC)
      Kernel::Units::timeConversionVector(*data, tof_unit, MICROSEC);
  }

  void loadDetid(H5::Group &event_group, std::unique_ptr<std::vector<detid_t>> &data,
                 const std::pair<uint64_t, uint64_t> &eventRange) {
    // g_log.information(NxsFieldNames::DETID);
    auto detID_SDS = event_group.openDataSet(NxsFieldNames::DETID);

    // H5Util resizes the vector
    const size_t offset = eventRange.first;
    const size_t slabsize = (eventRange.second == std::numeric_limits<size_t>::max())
                                ? std::numeric_limits<size_t>::max()
                                : eventRange.second - eventRange.first;
    NeXus::H5Util::readArray1DCoerce(detID_SDS, *data, slabsize, offset);
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
      // TODO this should be made smarter to only read the necessary range
      std::unique_ptr<std::vector<uint64_t>> event_index = std::make_unique<std::vector<uint64_t>>();
      this->loadEventIndex(event_group, event_index);

      uint64_t start_event = event_index->at(m_pulse_start_index);
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

template <typename Type> std::pair<Type, Type> parallel_minmax(const std::vector<Type> *vec, const size_t grainsize) {
  if (vec->size() < grainsize) {
    const auto [minval, maxval] = std::minmax_element(vec->cbegin(), vec->cend());
    return std::make_pair(*minval, *maxval);
  } else {
    MinMax<Type> finder(vec);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, vec->size(), grainsize), finder);
    return std::make_pair(finder.minval, finder.maxval);
  }
}

class ProcessEventsTask {
public:
  ProcessEventsTask(const std::vector<detid_t> *detids, const std::vector<float> *tofs,
                    const AlignAndFocusPowderSlim::BankCalibration *calibration, const std::vector<double> *binedges,
                    const std::set<detid_t> *masked)
      : m_detids(detids), m_tofs(tofs), m_calibration(calibration), m_binedges(binedges), masked(masked),
        no_mask(masked->empty()) {
    y_temp.resize(m_binedges->size() - 1, 0);
  }
  // Local histogram for this block/thread
  std::vector<uint32_t> y_temp;

  void operator()(const tbb::blocked_range<size_t> &range) {
    const auto &binedges = *m_binedges;
    const size_t nbins = binedges.size() - 1;

    for (size_t i = range.begin(); i < range.end(); ++i) {
      const detid_t &detid = (*m_detids)[i];
      if (no_mask || (masked->find(detid) == masked->end())) {
        // Apply calibration
        const double tof = static_cast<double>((*m_tofs)[i]) * m_calibration->value(detid);

        // Find the bin index using binary search
        const auto it = std::upper_bound(binedges.cbegin(), binedges.cend(), tof);
        const size_t bin = (it == binedges.cbegin() || it == binedges.cend())
                               ? nbins
                               : static_cast<size_t>(it - binedges.cbegin() - 1);

        if (bin < nbins) {
          y_temp[bin]++;
        }
      }
    }
  }

  ProcessEventsTask(ProcessEventsTask &other, tbb::split)
      : m_detids(other.m_detids), m_tofs(other.m_tofs), m_calibration(other.m_calibration),
        m_binedges(other.m_binedges), masked(other.masked), no_mask(other.no_mask) {
    y_temp.resize(m_binedges->size() - 1, 0);
  }

  void join(const ProcessEventsTask &other) {
    // Combine local histograms
    std::transform(y_temp.begin(), y_temp.end(), other.y_temp.cbegin(), y_temp.begin(), std::plus<>{});
  }

private:
  const std::vector<detid_t> *m_detids;
  const std::vector<float> *m_tofs;
  const AlignAndFocusPowderSlim::BankCalibration *m_calibration;
  const std::vector<double> *m_binedges;
  const std::set<detid_t> *masked;
  const bool no_mask; // whether there are any masked pixels
};

class ProcessBankTask {
public:
  ProcessBankTask(std::vector<std::string> &bankEntryNames, H5::H5File &h5file, const bool is_time_filtered,
                  const size_t pulse_start_index, const size_t pulse_stop_index, MatrixWorkspace_sptr &wksp,
                  const std::map<detid_t, double> &calibration, const std::set<detid_t> &masked,
                  const size_t events_per_chunk, const size_t grainsize_event, std::shared_ptr<API::Progress> &progress)
      : m_h5file(h5file), m_bankEntries(bankEntryNames),
        m_loader(is_time_filtered, pulse_start_index, pulse_stop_index), m_wksp(wksp), m_calibration(calibration),
        m_masked(masked), m_events_per_chunk(events_per_chunk), m_grainsize_event(grainsize_event),
        m_progress(progress) {
    if (false) { // H5Freopen_async(h5file.getId(), m_h5file.getId()) < 0) {
      throw std::runtime_error("failed to reopen async");
    }
  }

  void operator()(const tbb::blocked_range<size_t> &range) const {

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
                  << "   and should be read in " << (1 + (eventRangeFull.second / m_events_per_chunk)) << " chunks of "
                  << m_events_per_chunk << " (" << (m_events_per_chunk / 1024 / 1024) << "MB)"
                  << "\n";

        // create a histogrammer to process the events
        auto &spectrum = m_wksp->getSpectrum(wksp_index);

        // std::atomic allows for multi-threaded accumulation and who cares about floats when you are just
        // counting things
        std::vector<std::atomic_uint32_t> y_temp(spectrum.dataY().size());
        // std::vector<uint32_t> y_temp(spectrum.dataY().size());

        tbb::task_group tg;
        // read parts of the bank at a time
        size_t event_index_start = eventRangeFull.first;
        while (event_index_start < eventRangeFull.second) {
          // H5Cpp will truncate correctly
          const std::pair<uint64_t, uint64_t> eventRangePartial{event_index_start,
                                                                event_index_start + m_events_per_chunk};

          // load data and reuse chunk memory
          std::unique_ptr<std::vector<detid_t>> event_detid = std::make_unique<std::vector<detid_t>>();
          std::unique_ptr<std::vector<float>> event_time_of_flight = std::make_unique<std::vector<float>>();
          m_loader.loadTOF(event_group, event_time_of_flight, eventRangePartial);
          m_loader.loadDetid(event_group, event_detid, eventRangePartial);

          // process the events that were loaded
          const auto [minval, maxval] = parallel_minmax(event_detid.get(), m_grainsize_event);
          // only recreate if it doesn't already have the useful information
          if ((!calibration) || (calibration->idmin() > static_cast<detid_t>(minval)) ||
              (calibration->idmax() < static_cast<detid_t>(maxval))) {
            calibration = std::make_unique<AlignAndFocusPowderSlim::BankCalibration>(
                static_cast<detid_t>(minval), static_cast<detid_t>(maxval), m_calibration);
          }

          // Non-blocking processing of the events
          // Each thread needs its own ProcessEventsTask
          tg.run([event_detid = std::move(event_detid), event_time_of_flight = std::move(event_time_of_flight),
                  calibration = std::move(calibration), x_ptr = &spectrum.readX(), masked_ptr = &m_masked, &y_temp,
                  grainsize = m_grainsize_event]() {
            const auto numEvent = event_time_of_flight->size();

            // Create a local task for this thread
            ProcessEventsTask task(event_detid.get(), event_time_of_flight.get(), calibration.get(), x_ptr, masked_ptr);
            tbb::parallel_reduce(tbb::blocked_range<size_t>(0, numEvent, grainsize), task);
            // Atomically accumulate results into shared y_temp
            for (size_t i = 0; i < y_temp.size(); ++i) {
              y_temp[i].fetch_add(task.y_temp[i], std::memory_order_relaxed);
            }
          });

          event_index_start += m_events_per_chunk;
        }

        tg.wait();

        // copy the data out into the correct spectrum
        auto &y_values = spectrum.dataY();
        std::copy(y_temp.cbegin(), y_temp.cend(), y_values.begin());

        std::cout << bankName << " stop " << timer << std::endl;
      }
      m_progress->report();
    }
  }

private:
  H5::H5File m_h5file;
  const std::vector<std::string> m_bankEntries;
  mutable NexusLoader m_loader;
  MatrixWorkspace_sptr m_wksp;
  const std::map<detid_t, double> m_calibration; // detid: 1/difc
  const std::set<detid_t> m_masked;
  /// number of events to read from disk at one time
  const size_t m_events_per_chunk;
  /// number of events to histogram in a single thread
  const size_t m_grainsize_event;
  std::shared_ptr<API::Progress> m_progress;
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
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::FILTER_TIMESTART, EMPTY_DBL(),
                                                          Direction::Input),
      "To only include events after the provided start time, in seconds (relative to the start of the run).");

  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<double>>(PropertyNames::FILTER_TIMESTOP, EMPTY_DBL(),
                                                          Direction::Input),
      "To only include events before the provided stop time, in seconds (relative to the start of the run).");
  const std::vector<std::string> cal_exts{".h5", ".hd5", ".hdf", ".cal"};
  declareProperty(std::make_unique<FileProperty>(PropertyNames::CAL_FILE, "", FileProperty::OptionalLoad, cal_exts),
                  "The .cal file containing the position correction factors. Either this or OffsetsWorkspace needs to "
                  "be specified.");
  auto mustBePosArr = std::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::X_MIN, std::vector<double>{10}, mustBePosArr),
                  "Minimum x-value for the output binning");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::X_DELTA, std::vector<double>{0.0016}),
                  "Bin size for output data");
  declareProperty(
      std::make_unique<ArrayProperty<double>>(PropertyNames::X_MAX, std::vector<double>{16667}, mustBePosArr),
      "Minimum x-value for the output binning");
  declareProperty(std::make_unique<EnumeratedStringProperty<BinningMode, &binningModeNames>>(PropertyNames::BINMODE),
                  "Specify binning behavior ('Logarithmic')");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "An output workspace.");

  // parameters for chunking options - consider removing these later
  const std::string CHUNKING_PARAM_GROUP("Chunking-temporary");
  auto positiveIntValidator = std::make_shared<Mantid::Kernel::BoundedValidator<int>>();
  positiveIntValidator->setLower(1);
  declareProperty(std::make_unique<Kernel::PropertyWithValue<int>>(PropertyNames::READ_SIZE_FROM_DISK, 2000 * 50000,
                                                                   positiveIntValidator),
                  "Number of elements of time-of-flight or detector-id to read at a time. This is a maximum");
  setPropertyGroup(PropertyNames::READ_SIZE_FROM_DISK, CHUNKING_PARAM_GROUP);
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<int>>(PropertyNames::EVENTS_PER_THREAD, 1000000, positiveIntValidator),
      "Number of events to read in a single thread. Higher means less threads are created.");
  setPropertyGroup(PropertyNames::EVENTS_PER_THREAD, CHUNKING_PARAM_GROUP);
}

std::map<std::string, std::string> AlignAndFocusPowderSlim::validateInputs() {
  std::map<std::string, std::string> errors;

  // make sure that data is read in larger chunks than the events processed in a single thread
  const int disk_chunk = getProperty(PropertyNames::READ_SIZE_FROM_DISK);
  const int grainsize_events = getProperty(PropertyNames::EVENTS_PER_THREAD);
  if (disk_chunk < grainsize_events) {
    const std::string msg(PropertyNames::READ_SIZE_FROM_DISK + " must be larger than " +
                          PropertyNames::EVENTS_PER_THREAD);
    errors[PropertyNames::READ_SIZE_FROM_DISK] = msg;
    errors[PropertyNames::EVENTS_PER_THREAD] = msg;
  }

  const std::vector<double> xmins = getProperty(PropertyNames::X_MIN);
  const std::vector<double> xmaxs = getProperty(PropertyNames::X_MAX);
  const std::vector<double> deltas = getProperty(PropertyNames::X_DELTA);

  const auto numMin = xmins.size();
  const auto numMax = xmaxs.size();
  const auto numDelta = deltas.size();

  if (std::any_of(deltas.cbegin(), deltas.cend(), [](double d) { return !std::isfinite(d) || d == 0; }))
    errors[PropertyNames::X_DELTA] = "All must be nonzero";
  else if (!(numDelta == 1 || numDelta == NUM_HIST))
    errors[PropertyNames::X_DELTA] = "Must have 1 or 6 values";

  if (!(numMin == 1 || numMin == NUM_HIST))
    errors[PropertyNames::X_MIN] = "Must have 1 or 6 values";

  if (!(numMax == 1 || numMax == NUM_HIST))
    errors[PropertyNames::X_MAX] = "Must have 1 or 6 values";

  return errors;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AlignAndFocusPowderSlim::exec() {
  // create a histogram workspace

  // These give the limits in each file as to which events we actually load (when filtering by time).
  loadStart.resize(1, 0);
  loadSize.resize(1, 0);

  this->progress(.0, "Create output workspace");

  MatrixWorkspace_sptr wksp = createOutputWorkspace();

  const std::string filename = getPropertyValue(PropertyNames::FILENAME);
  { // TODO TEMPORARY - this algorithm is hard coded for VULCAN
    // it needs to be made more generic
    if (filename.find("VULCAN") == std::string::npos) {
      throw std::runtime_error("File does not appear to be for VULCAN");
    }
  }
  const Nexus::NexusDescriptor descriptor(filename);

  // instrument is needed for lots of things
  const std::string ENTRY_TOP_LEVEL("entry");
  LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, wksp, ENTRY_TOP_LEVEL, this, &descriptor);

  // TODO parameters should be input information
  const double l1{43.755};
  const std::vector<double> polars{90, 90, 120, 150, 157, 65.5}; // two-theta
  const std::vector<double> azimuthals{180, 0, 0, 0, 0, 0};      // angle from positive x-axis
  const std::vector<double> l2s{2.296, 2.296, 2.070, 2.070, 2.070, 2.530};
  const std::vector<specnum_t> specids;
  const auto difc_focused = calculate_difc_focused(l1, l2s, polars);

  // create values for focusing time-of-flight
  this->progress(.05, "Creating calibration constants");
  const std::string cal_filename = getPropertyValue(PropertyNames::CAL_FILE);
  if (!cal_filename.empty()) {
    this->loadCalFile(wksp, cal_filename, difc_focused);
  } else {
    this->initCalibrationConstants(wksp, difc_focused);
  }

  /* TODO create grouping information
  // create IndexInfo
  // prog->doReport("Creating IndexInfo"); TODO add progress bar stuff
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();
  */

  // load the events
  H5::H5File h5file(filename, H5F_ACC_RDONLY, NeXus::H5Util::defaultFileAcc());

  // filter by time
  double filter_time_start_sec = getProperty(PropertyNames::FILTER_TIMESTART);
  double filter_time_stop_sec = getProperty(PropertyNames::FILTER_TIMESTOP);
  if (filter_time_start_sec != EMPTY_DBL() || filter_time_stop_sec != EMPTY_DBL()) {
    this->progress(.15, "Creating time filtering");
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
    this->progress(.17, "Reading events");
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

    // each NXevent_data is a step
    const auto num_banks_to_read = bankEntryNames.size();

    // threaded processing of the banks
    const int DISK_CHUNK = getProperty(PropertyNames::READ_SIZE_FROM_DISK);
    const int GRAINSIZE_EVENTS = getProperty(PropertyNames::EVENTS_PER_THREAD);
    auto progress = std::make_shared<API::Progress>(this, .17, .9, num_banks_to_read);
    ProcessBankTask task(bankEntryNames, h5file, is_time_filtered, pulse_start_index, pulse_stop_index, wksp,
                         m_calibration, m_masked, static_cast<size_t>(DISK_CHUNK),
                         static_cast<size_t>(GRAINSIZE_EVENTS), progress);
    // generate threads only if appropriate
    if (num_banks_to_read > 1) {
      tbb::parallel_for(tbb::blocked_range<size_t>(0, num_banks_to_read), task);
    } else {
      task(tbb::blocked_range<size_t>(0, num_banks_to_read));
    }
  }

  // close the file so child algorithms can do their thing
  h5file.close();

  // set the instrument
  this->progress(.9, "Set instrument geometry");
  wksp = this->editInstrumentGeometry(wksp, l1, polars, specids, l2s, azimuthals);

  // load run metadata
  this->progress(.91, "Loading metadata");
  // prog->doReport("Loading metadata"); TODO add progress bar stuff
  try {
    LoadEventNexus::loadEntryMetadata(filename, wksp, ENTRY_TOP_LEVEL, descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // load logs
  this->progress(.92, "Loading logs");
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log"); // not used
  int nPeriods{1};
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, wksp, *this, false, nPeriods, periodLog);

  // set output units to be the same as coming from AlignAndFocusPowderFromFiles
  wksp->setYUnit("Counts");
  wksp->getAxis(0)->setUnit("TOF");
  setProperty(PropertyNames::OUTPUT_WKSP, std::move(wksp));
}

MatrixWorkspace_sptr AlignAndFocusPowderSlim::createOutputWorkspace() {
  // set up the output workspace binning
  const BINMODE binmode = getPropertyValue(PropertyNames::BINMODE);
  const bool linearBins = bool(binmode == BinningMode::LINEAR);
  std::vector<double> x_delta = getProperty(PropertyNames::X_DELTA);
  std::vector<double> x_min = getProperty(PropertyNames::X_MIN);
  std::vector<double> x_max = getProperty(PropertyNames::X_MAX);
  const bool raggedBins = (x_delta.size() != 1 || x_min.size() != 1 || x_max.size() != 1);

  constexpr bool resize_xnew{true};
  constexpr bool full_bins_only{false};

  // always use the first histogram x-values for initialization
  HistogramData::BinEdges XValues(0);
  if (linearBins) {
    const std::vector<double> params{x_min[0], x_delta[0], x_max[0]};
    UNUSED_ARG(
        Kernel::VectorHelper::createAxisFromRebinParams(params, XValues.mutableRawData(), resize_xnew, full_bins_only));
  } else {
    const std::vector<double> params{x_min[0], -1. * x_delta[0], x_max[0]};
    UNUSED_ARG(
        Kernel::VectorHelper::createAxisFromRebinParams(params, XValues.mutableRawData(), resize_xnew, full_bins_only));
  }
  MatrixWorkspace_sptr wksp = Mantid::DataObjects::create<Workspace2D>(NUM_HIST, XValues);

  if (raggedBins) {
    // if ragged bins, we need to resize the x-values for each histogram after the first one
    if (x_delta.size() == 1)
      x_delta.resize(NUM_HIST, x_delta[0]);
    if (x_min.size() == 1)
      x_min.resize(NUM_HIST, x_min[0]);
    if (x_max.size() == 1)
      x_max.resize(NUM_HIST, x_max[0]);

    for (size_t i = 1; i < NUM_HIST; ++i) {
      HistogramData::BinEdges XValues_new(0);

      if (linearBins) {
        const std::vector<double> params{x_min[i], x_delta[i], x_max[i]};
        Kernel::VectorHelper::createAxisFromRebinParams(params, XValues_new.mutableRawData(), resize_xnew,
                                                        full_bins_only);
      } else {
        const std::vector<double> params{x_min[i], -1. * x_delta[i], x_max[i]};
        Kernel::VectorHelper::createAxisFromRebinParams(params, XValues_new.mutableRawData(), resize_xnew,
                                                        full_bins_only);
      }
      HistogramData::Histogram hist(XValues_new, HistogramData::Counts(XValues_new.size() - 1, 0.0));
      wksp->setHistogram(i, hist);
    }
  }
  return wksp;
}

void AlignAndFocusPowderSlim::initCalibrationConstants(API::MatrixWorkspace_sptr &wksp,
                                                       const std::vector<double> &difc_focus) {
  const auto detInfo = wksp->detectorInfo();

  for (auto iter = detInfo.cbegin(); iter != detInfo.cend(); ++iter) {
    if (!iter->isMonitor()) {
      const auto difc_focussed = getFocussedPostion(static_cast<detid_t>(iter->detid()), difc_focus);
      m_calibration.emplace(iter->detid(), difc_focussed / detInfo.difcUncalibrated(iter->index()));
    }
  }
}

void AlignAndFocusPowderSlim::loadCalFile(const Mantid::API::Workspace_sptr &inputWS, const std::string &filename,
                                          const std::vector<double> &difc_focus) {
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
    const auto difc_focussed = getFocussedPostion(detid, difc_focus);
    m_calibration.emplace(detid, difc_focussed / detc);
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
