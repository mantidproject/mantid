// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadBankFromDiskTask.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidDataObjects/EventList.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/NexusIOHelper.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"

#include <atomic>
#include <regex>

namespace Mantid::DataHandling {
using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

namespace { // anonymous namespace
namespace PropertyNames {
const std::string FILENAME("Filename");
const std::string CAL_FILE("CalFileName");
const std::string LOAD_IDF_FROM_NXS("LoadNexusInstrumentXML");
const std::string OUTPUT_WKSP("OutputWorkspace");
} // namespace PropertyNames

namespace NxsFieldNames {
const std::string TIME_OF_FLIGHT("event_time_offset");
const std::string DETID("event_id");
} // namespace NxsFieldNames

// this is used for unit conversion to correct units
const std::string MICROSEC("microseconds");

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

template <typename CountsType> class ProcessEventsTask {
public:
  ProcessEventsTask(const Histogrammer *histogrammer, const std::vector<uint32_t> *detids,
                    const std::vector<float> *tofs, const AlignAndFocusPowderSlim::BankCalibration *calibration,
                    std::vector<CountsType> *y_temp)
      : m_histogrammer(histogrammer), m_detids(detids), m_tofs(tofs), m_calibration(calibration), y_temp(y_temp) {}

  void operator()(const tbb::blocked_range<size_t> &range) const {
    for (size_t i = range.begin(); i < range.end(); ++i) {
      const auto detid = static_cast<detid_t>(m_detids->at(i));
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
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AlignAndFocusPowderSlim::exec() {
  // create a histogram workspace
  constexpr size_t numHist{6};
  constexpr double xmin{6463};
  constexpr double xmax{39950};

  HistogramData::BinEdges XValues_new(0);
  const double binWidth{10.};
  const bool linearBins = bool(binWidth > 0.);
  UNUSED_ARG(Kernel::VectorHelper::createAxisFromRebinParams({xmin, binWidth, xmax}, XValues_new.mutableRawData(), true,
                                                             false, xmin, xmax));
  const size_t numBins = XValues_new.size() - 1;
  MatrixWorkspace_sptr wksp = WorkspaceFactory::Instance().create("Workspace2D", numHist, numBins + 1, numBins);
  for (size_t i = 0; i < numHist; ++i) {
    wksp->setBinEdges(i, XValues_new);
  }

  const std::string filename = getPropertyValue(PropertyNames::FILENAME);
  const Kernel::NexusHDF5Descriptor descriptor(filename);

  const std::string ENTRY_TOP_LEVEL("entry");

  // Load the instrument
  // prog->doReport("Loading instrument"); TODO add progress bar stuff
  // LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, wksp, "entry", this, &descriptor);
  LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, wksp, ENTRY_TOP_LEVEL, this, &descriptor);
  this->initCalibrationConstants(wksp);

  /*
  // load run metadata
  // prog->doReport("Loading metadata"); TODO add progress bar stuff
  try {
    LoadEventNexus::loadEntryMetadata(filename, WS, "entry", descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // create IndexInfo
  // prog->doReport("Creating IndexInfo"); TODO add progress bar stuff
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();

  // make output workspace with correct number of histograms
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(WS, numHist, 2, 1);
  // set spectrum index information
  outWS->setIndexInfo(indexInfo);
  */

  // load the events
  ::NeXus::File h5file(filename);

  h5file.openPath("/");
  h5file.openGroup(ENTRY_TOP_LEVEL, "NXentry"); // TODO should this allow other entries?

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  auto itClassEntries = allEntries.find("NXevent_data");

  // temporary "map" for detid -> calibration constant

  if (itClassEntries != allEntries.end()) {
    const std::set<std::string> &classEntries = itClassEntries->second;
    const std::regex classRegex("(/entry/)([^/]*)");
    std::smatch groups;

    // TODO should re-use vectors to save malloc/free calls
    std::unique_ptr<std::vector<uint32_t>> event_detid = std::make_unique<std::vector<uint32_t>>();
    std::unique_ptr<std::vector<float>> event_time_of_flight = std::make_unique<std::vector<float>>();
    // TODO std::unique_ptr<std::vector<float>> event_weight; some other time
    // std::unique_ptr<std::vector<uint64_t>> event_index; matching pulse-times with events

    size_t specnum = 0;
    for (const std::string &classEntry : classEntries) {
      if (std::regex_match(classEntry, groups, classRegex)) {
        const std::string entry_name(groups[2].str());
        const auto startTimeBank = std::chrono::high_resolution_clock::now();

        // skip entries with junk data
        if (entry_name == "bank_error_events" || entry_name == "bank_unmapped_events")
          continue;

        g_log.information() << "Loading bank " << entry_name << '\n';
        h5file.openGroup(entry_name, "NXevent_data");

        {
          const auto startTime = std::chrono::high_resolution_clock::now();
          loadTOF(event_time_of_flight, h5file);
          addTimer("readTOF" + entry_name, startTime, std::chrono::high_resolution_clock::now());
        }
        {
          const auto startTime = std::chrono::high_resolution_clock::now();
          loadDetid(event_detid, h5file);
          addTimer("readDetID" + entry_name, startTime, std::chrono::high_resolution_clock::now());
        }

        const auto startTimeSetup = std::chrono::high_resolution_clock::now();
        const auto [minval, maxval] = parallel_minmax(event_detid.get());
        BankCalibration calibration(static_cast<detid_t>(minval), static_cast<detid_t>(maxval), m_calibration);

        auto &spectrum = wksp->getSpectrum(specnum);
        Histogrammer histogrammer(&spectrum.readX(), binWidth, linearBins);
        const auto numEvent = event_time_of_flight->size();
        // std::atomic allows for multi-threaded accumulation and who cares about floats when you are just
        // counting things
        std::vector<std::atomic_uint32_t> y_temp(spectrum.dataY().size());
        addTimer("setup" + entry_name, startTimeSetup, std::chrono::high_resolution_clock::now());

        const auto startTimeProcess = std::chrono::high_resolution_clock::now();
        ProcessEventsTask task(&histogrammer, event_detid.get(), event_time_of_flight.get(), &calibration, &y_temp);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, numEvent), task);
        auto &y_values = spectrum.dataY();
        std::copy(y_temp.cbegin(), y_temp.cend(), y_values.begin());
        addTimer("proc" + entry_name, startTimeProcess, std::chrono::high_resolution_clock::now());
        addTimer(entry_name, startTimeBank, std::chrono::high_resolution_clock::now());

        h5file.closeGroup();
        specnum++;
      }
    }
  }

  // go back to where we started
  h5file.closeGroup();
  h5file.close();

  // TODO load logs

  setProperty("OutputWorkspace", std::move(wksp));
}

void AlignAndFocusPowderSlim::initCalibrationConstants(API::MatrixWorkspace_sptr &wksp) {
  const auto detInfo = wksp->detectorInfo();
  // TODO currently arbitrary
  const auto difCFocus = 1. / Kernel::Units::tofToDSpacingFactor(detInfo.l1(), 1., 0.5 * M_PI, 0.);

  for (auto iter = detInfo.cbegin(); iter != detInfo.cend(); ++iter) {
    if (!iter->isMonitor()) {
      m_calibration.emplace(iter->detid(), difCFocus / detInfo.difcUncalibrated(iter->index()));
    }
  }
}

void AlignAndFocusPowderSlim::loadTOF(std::unique_ptr<std::vector<float>> &data, ::NeXus::File &h5file) {
  g_log.information(NxsFieldNames::TIME_OF_FLIGHT);
  h5file.openData(NxsFieldNames::TIME_OF_FLIGHT);

  // This is the data size
  ::NeXus::Info id_info = h5file.getInfo();
  const auto dim0 = static_cast<size_t>(LoadBankFromDiskTask::recalculateDataSize(id_info.dims[0]));
  data->resize(dim0);

  Mantid::NeXus::NeXusIOHelper::readNexusVector<float>(*data, h5file, NxsFieldNames::TIME_OF_FLIGHT);

  // get the units
  std::string tof_unit;
  h5file.getAttr("units", tof_unit);

  // close the sds
  h5file.closeData();

  // Convert Tof to microseconds
  if (tof_unit != MICROSEC)
    Kernel::Units::timeConversionVector(*data, tof_unit, MICROSEC);
}

void AlignAndFocusPowderSlim::loadDetid(std::unique_ptr<std::vector<uint32_t>> &data, ::NeXus::File &h5file) {
  g_log.information(NxsFieldNames::DETID);
  h5file.openData(NxsFieldNames::DETID);

  // This is the data size
  ::NeXus::Info id_info = h5file.getInfo();
  const auto dim0 = static_cast<size_t>(LoadBankFromDiskTask::recalculateDataSize(id_info.dims[0]));
  data->resize(dim0);

  Mantid::NeXus::NeXusIOHelper::readNexusVector<uint32_t>(*data, h5file, NxsFieldNames::DETID);

  // close the sds
  h5file.closeData();
}

// ------------------------ BankCalibration object
AlignAndFocusPowderSlim::BankCalibration::BankCalibration(const detid_t idmin, const detid_t idmax,
                                                          const std::map<detid_t, double> &calibration_map)
    : m_detid_offset(idmin) {
  // error check the id-range
  if (idmax < idmin)
    throw std::runtime_error("BAD!"); // TODO better message

  std::cout << "Setting size " << static_cast<size_t>(idmax - idmin + 1) << "\n";
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
double AlignAndFocusPowderSlim::BankCalibration::value(const detid_t detid) const {
  return m_calibration[detid - m_detid_offset];
}

} // namespace Mantid::DataHandling
