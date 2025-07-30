// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "AlignAndFocusPowderSlim/ProcessBankTask.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TimeSplitter.h"
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

#include <H5Cpp.h>
#include <numbers>
#include <regex>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {
using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::Workspace2D;
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::EnumeratedStringProperty;
using Mantid::Kernel::TimeROI;
using Mantid::Kernel::TimeSeriesProperty;

namespace { // anonymous namespace
namespace PropertyNames {
const std::string FILENAME("Filename");
const std::string CAL_FILE("CalFileName");
const std::string FILTER_TIMESTART("FilterByTimeStart");
const std::string FILTER_TIMESTOP("FilterByTimeStop");
const std::string SPLITTER_WS("SplitterWorkspace");
const std::string SPLITTER_RELATIVE("RelativeTime");
const std::string SPLITTER_TARGET("SplitterTarget");
const std::string FILTER_BAD_PULSES("FilterBadPulses");
const std::string FILTER_BAD_PULSES_LOWER_CUTOFF("BadPulsesLowerCutoff");
const std::string X_MIN("XMin");
const std::string X_MAX("XMax");
const std::string X_DELTA("XDelta");
const std::string BIN_UNITS("BinningUnits");
const std::string BINMODE("BinningMode");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string READ_SIZE_FROM_DISK("ReadSizeFromDisk");
const std::string EVENTS_PER_THREAD("EventsPerThread");
} // namespace PropertyNames

const std::string LOG_CHARGE_NAME("proton_charge");

const std::vector<std::string> binningModeNames{"Logarithmic", "Linear"};
enum class BinningMode { LOGARITHMIC, LINEAR, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;

const std::vector<std::string> unitNames{"dSpacing", "TOF", "MomentumTransfer"};
enum class BinUnit { DSPACE, TOF, Q, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinUnit, &unitNames> BINUNIT;

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

std::vector<double> calculate_difc_focused(const double l1, const std::vector<double> &l2s,
                                           const std::vector<double> &polars) {
  constexpr double deg2rad = std::numbers::pi_v<double> / 180.;

  std::vector<double> difc;

  std::transform(l2s.cbegin(), l2s.cend(), polars.cbegin(), std::back_inserter(difc),
                 [l1, deg2rad](const auto &l2, const auto &polar) {
                   return 1. / Kernel::Units::tofToDSpacingFactor(l1, l2, deg2rad * polar, 0.);
                 });

  return difc;
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
  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      PropertyNames::SPLITTER_WS, "", Direction::Input, API::PropertyMode::Optional),
                  "Input workspace specifying \"splitters\", i.e. time intervals and targets for event filtering. "
                  "Currently only a single output workspace is supported.");
  declareProperty(PropertyNames::SPLITTER_RELATIVE, false,
                  "Flag indicating whether in SplitterWorkspace the times are absolute or "
                  "relative. If true, they are relative to the run start time.");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::SPLITTER_TARGET, 0, mustBePositive, "The target workspace index for the splitter.");
  declareProperty(PropertyNames::FILTER_BAD_PULSES, false,
                  "Filter bad pulses in the same way that :ref:`algm-FilterBadPulses` does.");
  auto range = std::make_shared<BoundedValidator<double>>();
  range->setBounds(0., 100.);
  declareProperty(PropertyNames::FILTER_BAD_PULSES_LOWER_CUTOFF, 95., range,
                  "The percentage of the average to use as the lower bound when filtering bad pulses.");
  const std::vector<std::string> cal_exts{".h5", ".hd5", ".hdf", ".cal"};
  declareProperty(std::make_unique<FileProperty>(PropertyNames::CAL_FILE, "", FileProperty::OptionalLoad, cal_exts),
                  "The .cal file containing the position correction factors. Either this or OffsetsWorkspace needs to "
                  "be specified.");
  auto mustBePosArr = std::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::X_MIN, std::vector<double>{0.1}, mustBePosArr),
                  "Minimum x-value for the output binning");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::X_DELTA, std::vector<double>{0.0016}),
                  "Bin size for output data");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::X_MAX, std::vector<double>{2.0}, mustBePosArr),
                  "Minimum x-value for the output binning");
  declareProperty(std::make_unique<EnumeratedStringProperty<BinUnit, &unitNames>>(PropertyNames::BIN_UNITS),
                  "The units of the input X min, max and delta values. Output will always be TOF");
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

  // set the instrument
  this->progress(.07, "Set instrument geometry");
  wksp = this->editInstrumentGeometry(wksp, l1, polars, specids, l2s, azimuthals);

  // convert to TOF if not already
  this->progress(.1, "Convert bins to TOF");
  wksp = this->convertToTOF(wksp);

  /* TODO create grouping information
  // create IndexInfo
  // prog->doReport("Creating IndexInfo"); TODO add progress bar stuff
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();
  */

  // load run metadata
  this->progress(.11, "Loading metadata");
  // prog->doReport("Loading metadata"); TODO add progress bar stuff
  try {
    LoadEventNexus::loadEntryMetadata(filename, wksp, ENTRY_TOP_LEVEL, descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // load logs
  this->progress(.12, "Loading logs");
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log"); // not used
  int nPeriods{1};
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, wksp, *this, false, nPeriods, periodLog);

  // determine the pulse indices from the time and splitter workspace
  this->progress(.15, "Determining pulse indices");
  this->determinePulseIndices(wksp);

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  auto itClassEntries = allEntries.find("NXevent_data");

  // load the events
  H5::H5File h5file(filename, H5F_ACC_RDONLY, NeXus::H5Util::defaultFileAcc());
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
    g_log.debug() << (DISK_CHUNK / GRAINSIZE_EVENTS) << " threads per chunk\n";
    ProcessBankTask task(bankEntryNames, h5file, is_time_filtered, wksp, m_calibration, m_masked,
                         static_cast<size_t>(DISK_CHUNK), static_cast<size_t>(GRAINSIZE_EVENTS), pulse_indices,
                         progress);
    // generate threads only if appropriate
    if (num_banks_to_read > 1) {
      tbb::parallel_for(tbb::blocked_range<size_t>(0, num_banks_to_read), task);
    } else {
      task(tbb::blocked_range<size_t>(0, num_banks_to_read));
    }
  }

  // close the file so child algorithms can do their thing
  h5file.close();

  setProperty(PropertyNames::OUTPUT_WKSP, std::move(wksp));
}

MatrixWorkspace_sptr AlignAndFocusPowderSlim::createOutputWorkspace() {
  // set up the output workspace binning
  const BINMODE binmode = getPropertyValue(PropertyNames::BINMODE);
  const bool linearBins = bool(binmode == BinningMode::LINEAR);
  const std::string binUnits = getPropertyValue(PropertyNames::BIN_UNITS);
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

  wksp->getAxis(0)->setUnit(binUnits);
  wksp->setYUnit("Counts");

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
  editAlg->setLoggingOffset(1);
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

API::MatrixWorkspace_sptr AlignAndFocusPowderSlim::convertToTOF(API::MatrixWorkspace_sptr &wksp) {
  if (wksp->getAxis(0)->unit()->unitID() == "TOF") {
    // already in TOF, no need to convert
    return wksp;
  }

  API::IAlgorithm_sptr convertUnits = createChildAlgorithm("ConvertUnits");
  convertUnits->setProperty("InputWorkspace", wksp);
  convertUnits->setPropertyValue("Target", "TOF");
  convertUnits->executeAsChildAlg();
  wksp = convertUnits->getProperty("OutputWorkspace");

  return wksp;
}

void AlignAndFocusPowderSlim::determinePulseIndices(const API::MatrixWorkspace_sptr &wksp) {
  TimeROI roi;
  const auto startOfRun = wksp->run().startTime();

  // filter by time
  double filter_time_start_sec = getProperty(PropertyNames::FILTER_TIMESTART);
  double filter_time_stop_sec = getProperty(PropertyNames::FILTER_TIMESTOP);
  if (filter_time_start_sec != EMPTY_DBL() || filter_time_stop_sec != EMPTY_DBL()) {
    this->progress(.15, "Creating time filtering");
    g_log.information() << "Filtering pulses from " << filter_time_start_sec << " to " << filter_time_stop_sec << "s\n";

    try {
      roi.addROI(startOfRun + (filter_time_start_sec == EMPTY_DBL() ? 0.0 : filter_time_start_sec),
                 startOfRun + filter_time_stop_sec); // start and stop times in seconds
    } catch (const std::runtime_error &e) {
      throw std::invalid_argument("Invalid time range for filtering: " + std::string(e.what()));
    }
  }

  // filter bad pulses
  if (getProperty(PropertyNames::FILTER_BAD_PULSES)) {
    this->progress(.16, "Filtering bad pulses");

    // get limits from proton_charge
    const auto [min_pcharge, max_pcharge, mean] =
        wksp->run().getBadPulseRange(LOG_CHARGE_NAME, getProperty(PropertyNames::FILTER_BAD_PULSES_LOWER_CUTOFF));
    g_log.information() << "Filtering bad pulses; pcharge outside of " << min_pcharge << " to " << max_pcharge << '\n';

    const auto run_start = wksp->getFirstPulseTime();
    const auto run_stop = wksp->getLastPulseTime();

    const auto log = dynamic_cast<const TimeSeriesProperty<double> *>(wksp->run().getLogData(LOG_CHARGE_NAME));
    if (log) {
      roi = log->makeFilterByValue(min_pcharge, max_pcharge, true, Mantid::Kernel::TimeInterval(run_start, run_stop),
                                   0.0, true, &roi);
    }
  }

  // filter by splitter workspace
  const auto splitter_roi = timeROIFromSplitterWorkspace(startOfRun);

  if (roi.useAll())
    roi = splitter_roi; // use the splitter ROI if no time filtering is specified
  else if (!splitter_roi.useAll())
    roi.update_intersection(splitter_roi); // otherwise intersect with the splitter ROI

  if (roi.useAll()) {
    pulse_indices.emplace_back(0, std::numeric_limits<size_t>::max());
  } else {
    is_time_filtered = true;

    // get pulse times from frequency log on workspace
    const auto frequency_log = dynamic_cast<const TimeSeriesProperty<double> *>(wksp->run().getProperty("frequency"));
    if (!frequency_log) {
      throw std::runtime_error("Frequency log not found in workspace run");
    }
    const auto pulse_times =
        std::make_unique<std::vector<Mantid::Types::Core::DateAndTime>>(frequency_log->timesAsVector());

    pulse_indices = roi.calculate_indices(*pulse_times);
    if (pulse_indices.empty())
      throw std::invalid_argument("No valid pulse time indices found for filtering");
  }

  // update the run TimeROI and remove log data outside the time ROI
  wksp->mutableRun().setTimeROI(roi);
  wksp->mutableRun().removeDataOutsideTimeROI();
}

TimeROI AlignAndFocusPowderSlim::timeROIFromSplitterWorkspace(const Types::Core::DateAndTime &filterStartTime) {
  API::Workspace_sptr tempws = this->getProperty("SplitterWorkspace");
  DataObjects::SplittersWorkspace_sptr splittersWorkspace =
      std::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(tempws);
  DataObjects::TableWorkspace_sptr splitterTableWorkspace =
      std::dynamic_pointer_cast<DataObjects::TableWorkspace>(tempws);
  API::MatrixWorkspace_sptr matrixSplitterWS = std::dynamic_pointer_cast<API::MatrixWorkspace>(tempws);

  if (!splittersWorkspace && !splitterTableWorkspace && !matrixSplitterWS)
    return TimeROI();

  const bool isSplittersRelativeTime = this->getProperty("RelativeTime");

  DataObjects::TimeSplitter timeSplitter;
  if (splittersWorkspace) {
    timeSplitter = DataObjects::TimeSplitter{splittersWorkspace};
  } else if (splitterTableWorkspace) {
    timeSplitter = DataObjects::TimeSplitter(splitterTableWorkspace,
                                             isSplittersRelativeTime ? filterStartTime : DateAndTime::GPS_EPOCH);
  } else {
    timeSplitter =
        DataObjects::TimeSplitter(matrixSplitterWS, isSplittersRelativeTime ? filterStartTime : DateAndTime::GPS_EPOCH);
  }

  const int splitter_target = this->getProperty(PropertyNames::SPLITTER_TARGET);
  if (!timeSplitter.outputWorkspaceIndices().contains(splitter_target)) {
    throw std::invalid_argument("Selected splitter target is out of range.");
  }

  return timeSplitter.getTimeROI(splitter_target);
}

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
