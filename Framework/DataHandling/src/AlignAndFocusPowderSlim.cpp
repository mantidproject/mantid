// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankSplitFullTimeTask.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankSplitTask.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankTask.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TimeSplitter.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"

#include <H5Cpp.h>
#include <numbers>
#include <ranges>
#include <regex>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {
using Mantid::API::AnalysisDataService;
using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::TimeSplitter;
using Mantid::DataObjects::Workspace2D;
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::EnumeratedStringProperty;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::TimeROI;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::Strings::strmakef;

namespace { // anonymous namespace

const std::string LOG_CHARGE_NAME("proton_charge");

const std::vector<std::string> binningModeNames{"Logarithmic", "Linear"};
enum class BinningMode { LOGARITHMIC, LINEAR, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;

const std::vector<std::string> unitNames{"dSpacing", "TOF", "MomentumTransfer"};
enum class BinUnit { DSPACE, TOF, Q, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinUnit, &unitNames> BINUNIT;

const size_t NUM_HIST{6}; // TODO make this determined from groupin

// TODO refactor this to use the actual grouping
double getFocussedPostion(const detid_t detid, const std::vector<double> &difc_focus,
                          std::map<detid_t, size_t> &detIDToSpecNum) {
  if (detIDToSpecNum.contains(detid)) {
    return difc_focus[detIDToSpecNum[detid]];
  } else {
    throw std::runtime_error("detid " + std::to_string(detid) + " not found in detIDToSpecNum map.");
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
      std::make_unique<PropertyWithValue<double>>(PropertyNames::FILTER_TIMESTART, EMPTY_DBL(), Direction::Input),
      "To only include events after the provided start time, in seconds (relative to the start of the run).");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(PropertyNames::FILTER_TIMESTOP, EMPTY_DBL(), Direction::Input),
      "To only include events before the provided stop time, in seconds (relative to the start of the run).");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      PropertyNames::SPLITTER_WS, "", Direction::Input, API::PropertyMode::Optional),
                  "Input workspace specifying \"splitters\", i.e. time intervals and targets for event filtering. "
                  "Currently only a single output workspace is supported.");
  declareProperty(PropertyNames::SPLITTER_RELATIVE, false,
                  "Flag indicating whether in SplitterWorkspace the times are absolute or "
                  "relative. If true, they are relative to the run start time.");
  declareProperty(
      PropertyNames::PROCESS_BANK_SPLIT_TASK, false,
      "For development testing. Changes how the splitters are processed. If true then use ProcessBankSplitTask "
      "otherwise loop over ProcessBankTask.");
  declareProperty(PropertyNames::CORRECTION_TO_SAMPLE, false,
                  "Find time-of-flight when neutron was at the sample position. This is only necessary for fast logs "
                  "(i.e. more frequent than proton on target pulse).");
  declareProperty(
      PropertyNames::FULL_TIME, false,
      "If true, events will be splitting using full time values (tof+pulsetime) rather than just pulsetime.");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::FILTER_BAD_PULSES, false,
                  "Filter bad pulses in the same way that :ref:`algm-FilterBadPulses` does.");
  auto range = std::make_shared<BoundedValidator<double>>();
  range->setBounds(0., 100.);
  declareProperty(PropertyNames::FILTER_BAD_PULSES_LOWER_CUTOFF, 95., range,
                  "The percentage of the average to use as the lower bound when filtering bad pulses.");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::GroupingWorkspace>>(
                      PropertyNames::GROUPING_WS, "", Direction::Input, API::PropertyMode::Optional),
                  "A GroupingWorkspace giving the grouping info. If not provided then the grouping is one per bank. "
                  "You need to provide");
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
  declareProperty(std::make_unique<ArrayProperty<std::string>>(PropertyNames::ALLOW_LOGS),
                  "If specified, only these logs will be loaded from the file");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(PropertyNames::BLOCK_LOGS),
                  "If specified, these logs will not be loaded from the file");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::Workspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "An output workspace.");

  // parameters for chunking options - consider removing these later
  const std::string CHUNKING_PARAM_GROUP("Chunking-temporary");
  auto positiveIntValidator = std::make_shared<Mantid::Kernel::BoundedValidator<int>>();
  positiveIntValidator->setLower(1);
  declareProperty(
      std::make_unique<PropertyWithValue<int>>(PropertyNames::READ_SIZE_FROM_DISK, 2000 * 50000, positiveIntValidator),
      "Number of elements of time-of-flight or detector-id to read at a time. This is a maximum");
  setPropertyGroup(PropertyNames::READ_SIZE_FROM_DISK, CHUNKING_PARAM_GROUP);
  declareProperty(
      std::make_unique<PropertyWithValue<int>>(PropertyNames::EVENTS_PER_THREAD, 1000000, positiveIntValidator),
      "Number of events to read in a single thread. Higher means less threads are created.");
  setPropertyGroup(PropertyNames::EVENTS_PER_THREAD, CHUNKING_PARAM_GROUP);

  // load single spectrum
  declareProperty(
      std::make_unique<PropertyWithValue<int>>(PropertyNames::OUTPUT_SPEC_NUM, EMPTY_INT(), positiveIntValidator),
      "The bank for which to read data; if specified, others will be blank");

  // parameters for focus position
  // for L1, mandatory and must be positive
  auto mandatoryDblValidator = std::make_shared<MandatoryValidator<double>>();
  auto positiveDblValidator = std::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  positiveDblValidator->setLower(0);
  auto l1Validator = std::make_shared<CompositeValidator>();
  l1Validator->add(mandatoryDblValidator);
  l1Validator->add(positiveDblValidator);
  // for L2, 2theta, phi, mandatory arrays with positive valyes
  auto mandatoryDblArrayValidator = std::make_shared<MandatoryValidator<std::vector<double>>>();
  auto positionArrayValidator = std::make_shared<CompositeValidator>();
  positionArrayValidator->add(mandatoryDblArrayValidator);
  positionArrayValidator->add(mustBePosArr);
  declareProperty(std::make_unique<PropertyWithValue<double>>(PropertyNames::L1, EMPTY_DBL(), l1Validator),
                  "The primary distance :math:`\\ell_1` from beam to sample");
  declareProperty(
      std::make_unique<ArrayProperty<double>>(PropertyNames::L2, std::vector<double>{}, positionArrayValidator),
      "The secondary distances :math:`\\ell_2` from sample to focus group");
  declareProperty(
      std::make_unique<ArrayProperty<double>>(PropertyNames::POLARS, std::vector<double>{}, positionArrayValidator),
      "The effective polar angle (:math:`2\\theta`) of each focus group");
  declareProperty(
      std::make_unique<ArrayProperty<double>>(PropertyNames::AZIMUTHALS, std::vector<double>{}, mustBePosArr),
      "The effective azimuthal angle :math:`\\phi` for each focus group");
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

  // For now only support either grouping or splitter workspace, not both at the same time
  if ((!isDefault(PropertyNames::GROUPING_WS)) && (!isDefault(PropertyNames::SPLITTER_WS))) {
    errors[PropertyNames::GROUPING_WS] = "Cannot specify both grouping and splitter workspaces";
    errors[PropertyNames::SPLITTER_WS] = "Cannot specify both grouping and splitter workspaces";
  }

  size_t num_hist = NUM_HIST;
  if (!isDefault(PropertyNames::GROUPING_WS)) {
    DataObjects::GroupingWorkspace_const_sptr groupingWS = this->getProperty(PropertyNames::GROUPING_WS);
    const auto groupIds = groupingWS->getGroupIDs(false);
    num_hist = groupIds.size();
  }

  // validate binning information is consistent with each other
  const std::vector<double> xmins = getProperty(PropertyNames::X_MIN);
  const std::vector<double> xmaxs = getProperty(PropertyNames::X_MAX);
  const std::vector<double> deltas = getProperty(PropertyNames::X_DELTA);

  const auto numMin = xmins.size();
  const auto numMax = xmaxs.size();
  const auto numDelta = deltas.size();

  if (std::any_of(deltas.cbegin(), deltas.cend(), [](double d) { return !std::isfinite(d) || d == 0; }))
    errors[PropertyNames::X_DELTA] = "All must be nonzero";
  else if (!(numDelta == 1 || numDelta == num_hist))
    errors[PropertyNames::X_DELTA] = "Must have 1 or " + std::to_string(num_hist) + " values";

  if (!(numMin == 1 || numMin == num_hist))
    errors[PropertyNames::X_MIN] = "Must have 1 or " + std::to_string(num_hist) + " values";

  if (!(numMax == 1 || numMax == num_hist))
    errors[PropertyNames::X_MAX] = "Must have 1 or " + std::to_string(num_hist) + " values";

  // only specify allow or block list for logs
  if ((!isDefault(PropertyNames::ALLOW_LOGS)) && (!isDefault(PropertyNames::BLOCK_LOGS))) {
    errors[PropertyNames::ALLOW_LOGS] = "Cannot specify both allow and block lists";
    errors[PropertyNames::BLOCK_LOGS] = "Cannot specify both allow and block lists";
  }

  // the focus group position parameters must have same lengths
  std::vector<double> l2s = getProperty(PropertyNames::L2);
  std::vector<double> twoTheta = getProperty(PropertyNames::POLARS);
  if (l2s.size() != twoTheta.size()) {
    errors[PropertyNames::L2] = strmakef("L2S has inconsistent length %zu", l2s.size());
    errors[PropertyNames::POLARS] = strmakef("Polar has inconsistent length %zu", twoTheta.size());
  }
  // phi is optional, but if set must also have same size
  std::vector<double> phi = getProperty(PropertyNames::AZIMUTHALS);
  if (!phi.empty()) {
    if (l2s.size() != phi.size()) {
      errors[PropertyNames::L2] = strmakef("L2S has inconsistent length %zu", l2s.size());
      errors[PropertyNames::AZIMUTHALS] = strmakef("Azimuthal has inconsistent length %zu", phi.size());
      ;
    }
  }

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

  size_t num_hist = NUM_HIST;
  bool arbitrary_grouping = false; // if false then we have a direct mapping of bank to output spectrum
  std::map<size_t, std::set<detid_t>> grouping;
  DataObjects::GroupingWorkspace_sptr groupingWS = this->getProperty(PropertyNames::GROUPING_WS);
  if (groupingWS) {
    arbitrary_grouping = true; // any bank can go to any output spectrum
    const auto groupIds = groupingWS->getGroupIDs(false);
    num_hist = groupIds.size();
    g_log.information() << "Using grouping workspace with " << num_hist << " groups\n";
    for (size_t outputSpecNum = 0; outputSpecNum < groupIds.size(); ++outputSpecNum) {
      const auto detids = groupingWS->getDetectorIDsOfGroup(groupIds[outputSpecNum]);
      grouping[outputSpecNum] = std::set<detid_t>(detids.begin(), detids.end());
    }
  }

  this->progress(.0, "Create output workspace");

  MatrixWorkspace_sptr wksp = createOutputWorkspace(num_hist);

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
  const double l1 = getProperty(PropertyNames::L1);
  const std::vector<double> l2s = getProperty(PropertyNames::L2);
  const std::vector<double> polars = getProperty(PropertyNames::POLARS); // two-theta
  // set angle from positive x-axis; will be zero unless specified
  std::vector<double> setPhi(l2s.size(), 0.0);
  if (!isDefault(PropertyNames::AZIMUTHALS)) {
    setPhi = getProperty(PropertyNames::AZIMUTHALS);
  }
  const std::vector<double> azimuthals(setPhi);
  const std::vector<specnum_t> specids;
  const auto difc_focused = calculate_difc_focused(l1, l2s, polars);

  // load run metadata
  this->progress(.01, "Loading metadata");
  // prog->doReport("Loading metadata"); TODO add progress bar stuff
  try {
    LoadEventNexus::loadEntryMetadata(filename, wksp, ENTRY_TOP_LEVEL, descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // load logs
  this->progress(.02, "Loading logs");
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log"); // not used
  const std::vector<std::string> &allow_logs = getProperty(PropertyNames::ALLOW_LOGS);
  const std::vector<std::string> &block_logs = getProperty(PropertyNames::BLOCK_LOGS);
  int nPeriods{1};
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, wksp, *this, false, nPeriods, periodLog, allow_logs,
                                                         block_logs);

  const auto timeSplitter = this->timeSplitterFromSplitterWorkspace(wksp->run().startTime());
  const auto filterROI = this->getFilterROI(wksp);
  // determine the pulse indices from the time and splitter workspace
  this->progress(.05, "Determining pulse indices");

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  auto itClassEntries = allEntries.find("NXevent_data");

  // load the events
  H5::H5File h5file(filename, H5F_ACC_RDONLY, Nexus::H5Util::defaultFileAcc());
  if (itClassEntries == allEntries.end()) {
    h5file.close();
    throw std::runtime_error("No NXevent_data entries found in file");
  }

  this->progress(.07, "Reading events");

  // hard coded for VULCAN 6 banks
  std::vector<std::string> bankEntryNames;
  std::size_t num_banks_to_read;

  int outputSpecNum = getProperty(PropertyNames::OUTPUT_SPEC_NUM);
  if (outputSpecNum == EMPTY_INT()) {
    for (size_t i = 1; i <= NUM_HIST; ++i) {
      bankEntryNames.push_back("bank" + std::to_string(i) + "_events");
    }
    num_banks_to_read = NUM_HIST;
  } else {
    // fill this vector with blanks -- this is for the ProcessBankTask to correctly access it
    for (size_t i = 1; i <= NUM_HIST; ++i) {
      bankEntryNames.push_back("");
    }
    // the desired bank gets the correct entry name
    bankEntryNames[outputSpecNum - 1] = "bank" + std::to_string(outputSpecNum) + "_events";
    num_banks_to_read = 1;
  }

  // get detector ids for each bank
  std::map<size_t, std::set<detid_t>> bank_detids;
  for (size_t i = 1; i <= NUM_HIST; ++i) {
    bank_detids[i - 1] = wksp->getInstrument()->getDetectorIDsInBank("bank" + std::to_string(i));
  }

  // create map of detid to output spectrum number to be used in focusing
  if (!grouping.empty()) {
    for (const auto &group : grouping) {
      for (const auto &detid : group.second) {
        detIDToSpecNum[detid] = group.first;
      }
    }
  } else {
    for (const auto &[i, detids] : bank_detids) {
      for (const auto &detid : detids) {
        detIDToSpecNum[detid] = i;
      }
    }
  }

  // create values for focusing time-of-flight
  this->progress(.1, "Creating calibration constants");
  const std::string cal_filename = getPropertyValue(PropertyNames::CAL_FILE);
  if (!cal_filename.empty()) {
    this->loadCalFile(wksp, cal_filename, difc_focused);
  } else {
    this->initCalibrationConstants(wksp, difc_focused);
  }

  // calculate correction for tof of the neutron at the sample position
  if (this->getProperty(PropertyNames::FULL_TIME)) {
    this->initScaleAtSample(wksp);
  }

  // set the instrument. Needs to happen after we get detector ids for each bank
  this->progress(.15, "Set instrument geometry");
  wksp = this->editInstrumentGeometry(wksp, l1, polars, specids, l2s, azimuthals);

  // convert to TOF if not already
  this->progress(.17, "Convert bins to TOF");
  wksp = this->convertToTOF(wksp);

  // create the bank calibration factory to share with all of the ProcessBank*Task objects
  BankCalibrationFactory calibFactory(m_calibration, m_scale_at_sample, grouping, m_masked, bank_detids);

  // threaded processing of the banks
  const int DISK_CHUNK = getProperty(PropertyNames::READ_SIZE_FROM_DISK);
  const int GRAINSIZE_EVENTS = getProperty(PropertyNames::EVENTS_PER_THREAD);
  g_log.debug() << (DISK_CHUNK / GRAINSIZE_EVENTS) << " threads per chunk\n";

  if (timeSplitter.empty()) {
    // create the nexus loader for handling combined calls to hdf5

    SpectraProcessingData processingData = initializeSpectraProcessingData(wksp);
    processingData.arbitraryGrouping = arbitrary_grouping;
    const auto pulse_indices = this->determinePulseIndices(wksp, filterROI);
    auto loader = std::make_shared<NexusLoader>(is_time_filtered, pulse_indices);

    auto progress = std::make_shared<API::Progress>(this, .17, .9, num_banks_to_read);
    ProcessBankTask task(bankEntryNames, h5file, loader, processingData, calibFactory, static_cast<size_t>(DISK_CHUNK),
                         static_cast<size_t>(GRAINSIZE_EVENTS), progress);
    // generate threads only if appropriate
    if (num_banks_to_read > 1) {
      tbb::parallel_for(tbb::blocked_range<size_t>(0, num_banks_to_read), task);
    } else {
      // a "range" of 1; note -1 to match 0-indexed array with 1-indexed bank labels
      task(tbb::blocked_range<size_t>(outputSpecNum - 1, outputSpecNum));
    }

    // close the file so child algorithms can do their thing
    h5file.close();

    // copy data from processingData to wksp
    storeSpectraProcessingData(processingData, wksp);

    // update the run TimeROI and remove log data outside the time ROI
    wksp->mutableRun().setTimeROI(filterROI);
    wksp->mutableRun().removeDataOutsideTimeROI();

    setProperty(PropertyNames::OUTPUT_WKSP, std::move(wksp));
  } else {
    std::string ws_basename = this->getPropertyValue(PropertyNames::OUTPUT_WKSP);
    std::vector<std::string> wsNames;
    std::vector<int> workspaceIndices;
    std::vector<MatrixWorkspace_sptr> workspaces;
    for (const int &splitter_target : timeSplitter.outputWorkspaceIndices()) {
      std::string ws_name = ws_basename + "_" + timeSplitter.getWorkspaceIndexName(splitter_target);
      wsNames.push_back(std::move(ws_name));
      workspaceIndices.push_back(splitter_target);
      workspaces.emplace_back(wksp->clone());
    }

    auto progress = std::make_shared<API::Progress>(this, .17, .9, num_banks_to_read * workspaceIndices.size());
    if (this->getProperty(PropertyNames::FULL_TIME)) {
      g_log.information() << "Using ProcessBankSplitFullTimeTask for splitter processing\n";

      // Get the combined time ROI for all targets so we only load necessary events.
      // Need to offset the start time to account for tof's greater than pulsetime. 66.6ms is 4 pulses.
      auto combined_time_roi = timeSplitter.combinedTimeROI(PULSETIME_OFFSET);
      if (!filterROI.useAll()) {
        combined_time_roi.update_intersection(filterROI);
      }

      // create the nexus loader for handling combined calls to hdf5
      const auto pulse_indices = this->determinePulseIndices(wksp, combined_time_roi);
      auto loader = std::make_shared<NexusLoader>(is_time_filtered, pulse_indices);

      const auto &splitterMap = timeSplitter.getSplittersMap();

      ProcessBankSplitFullTimeTask task(bankEntryNames, h5file, loader, workspaceIndices, workspaces, calibFactory,
                                        static_cast<size_t>(DISK_CHUNK), static_cast<size_t>(GRAINSIZE_EVENTS),
                                        splitterMap, progress);

      // generate threads only if appropriate
      if (num_banks_to_read > 1) {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, num_banks_to_read), task);
      } else {
        // a "range" of 1; note -1 to match 0-indexed array with 1-indexed bank labels
        task(tbb::blocked_range<size_t>(outputSpecNum - 1, outputSpecNum));
      }

    } else if (this->getProperty(PropertyNames::PROCESS_BANK_SPLIT_TASK)) {
      g_log.information() << "Using ProcessBankSplitTask for splitter processing\n";
      // determine the pulse indices from the time and splitter workspace
      const auto target_to_pulse_indices = this->determinePulseIndicesTargets(wksp, filterROI, timeSplitter);
      // create the nexus loader for handling combined calls to hdf5
      std::vector<PulseROI> pulse_indices; // intentionally empty to get around loader needing const reference
      auto loader = std::make_shared<NexusLoader>(is_time_filtered, pulse_indices, target_to_pulse_indices);

      ProcessBankSplitTask task(bankEntryNames, h5file, loader, workspaceIndices, workspaces, calibFactory,
                                static_cast<size_t>(DISK_CHUNK), static_cast<size_t>(GRAINSIZE_EVENTS), progress);
      // generate threads only if appropriate
      if (num_banks_to_read > 1) {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, num_banks_to_read), task);
      } else {
        // a "range" of 1; note -1 to match 0-indexed array with 1-indexed bank labels
        task(tbb::blocked_range<size_t>(outputSpecNum - 1, outputSpecNum));
      }
    } else {
      g_log.information() << "Using ProcessBankTask for splitter processing\n";
      // loop over the targets in the splitter workspace, each target gets its own output workspace
      tbb::parallel_for(
          tbb::blocked_range<size_t>(0, workspaceIndices.size()),
          [&](const tbb::blocked_range<size_t> &target_indices) {
            for (size_t target_index = target_indices.begin(); target_index != target_indices.end(); ++target_index) {
              const int splitter_target = workspaceIndices[target_index];

              auto splitter_roi = timeSplitter.getTimeROI(splitter_target);
              // copy the roi so we can modify it just for this target
              auto target_roi = filterROI;
              if (target_roi.useAll())
                target_roi = std::move(splitter_roi); // use the splitter ROI if no time filtering is specified
              else if (!splitter_roi.useAll())
                target_roi.update_intersection(splitter_roi); // otherwise intersect with the splitter ROI

              // clone wksp for this target
              MatrixWorkspace_sptr target_wksp = workspaces[target_index];
              SpectraProcessingData processingData = initializeSpectraProcessingData(target_wksp);

              const auto pulse_indices = this->determinePulseIndices(target_wksp, target_roi);
              auto loader = std::make_shared<NexusLoader>(is_time_filtered, pulse_indices);

              ProcessBankTask task(bankEntryNames, h5file, loader, processingData, calibFactory,
                                   static_cast<size_t>(DISK_CHUNK), static_cast<size_t>(GRAINSIZE_EVENTS), progress);
              // generate threads only if appropriate
              if (num_banks_to_read > 1) {
                tbb::parallel_for(tbb::blocked_range<size_t>(0, num_banks_to_read), task);
              } else {
                // a "range" of 1; note -1 to match 0-indexed array with 1-indexed bank labels
                task(tbb::blocked_range<size_t>(outputSpecNum - 1, outputSpecNum));
              }
              storeSpectraProcessingData(processingData, target_wksp);
            }
          });
    }

    // close the file so child algorithms can do their thing
    h5file.close();

    // add the workspaces to the ADS
    for (size_t idx = 0; idx < workspaceIndices.size(); ++idx) {
      // create the target time ROI combining the splitter and filter ROIs
      auto target_roi = timeSplitter.getTimeROI(workspaceIndices[idx]);
      if (target_roi.useAll())
        target_roi = filterROI; // use the splitter ROI if no time filtering is specified
      else if (!filterROI.useAll())
        target_roi.update_intersection(filterROI); // otherwise intersect with the splitter ROI

      // update the run TimeROI and remove log data outside the time ROI
      workspaces[idx]->mutableRun().setTimeROI(target_roi);
      workspaces[idx]->mutableRun().removeDataOutsideTimeROI();
      AnalysisDataService::Instance().addOrReplace(wsNames[idx], workspaces[idx]);
    }

    // group the workspaces
    auto groupws = createChildAlgorithm("GroupWorkspaces", 0.95, 1.00, true);
    groupws->setAlwaysStoreInADS(true);
    groupws->setProperty("InputWorkspaces", wsNames);
    groupws->setProperty("OutputWorkspace", ws_basename);
    groupws->execute();

    if (!groupws->isExecuted()) {
      throw std::runtime_error("Failed to group output workspaces");
    }

    API::Workspace_sptr outputWorkspace = AnalysisDataService::Instance().retrieveWS<API::Workspace>(ws_basename);

    setProperty(PropertyNames::OUTPUT_WKSP, outputWorkspace);
  }
}

MatrixWorkspace_sptr AlignAndFocusPowderSlim::createOutputWorkspace(size_t num_hist) {
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
  MatrixWorkspace_sptr wksp = Mantid::DataObjects::create<Workspace2D>(num_hist, XValues);

  if (raggedBins) {
    // if ragged bins, we need to resize the x-values for each histogram after the first one
    if (x_delta.size() == 1)
      x_delta.resize(num_hist, x_delta[0]);
    if (x_min.size() == 1)
      x_min.resize(num_hist, x_min[0]);
    if (x_max.size() == 1)
      x_max.resize(num_hist, x_max[0]);

    for (size_t i = 1; i < num_hist; ++i) {
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

SpectraProcessingData
AlignAndFocusPowderSlim::initializeSpectraProcessingData(const API::MatrixWorkspace_sptr &outputWS) {
  SpectraProcessingData processingData;
  const size_t numSpectra = outputWS->getNumberHistograms();
  for (size_t i = 0; i < numSpectra; ++i) {
    const auto &spectrum = outputWS->getSpectrum(i);
    processingData.binedges.emplace_back(&spectrum.readX());
    processingData.counts.emplace_back(spectrum.dataY().size());
  }
  return processingData;
}

void AlignAndFocusPowderSlim::storeSpectraProcessingData(const SpectraProcessingData &processingData,
                                                         const API::MatrixWorkspace_sptr &outputWS) {
  const size_t numSpectra = outputWS->getNumberHistograms();
  for (size_t i = 0; i < numSpectra; ++i) {
    auto &spectrum = outputWS->getSpectrum(i);
    auto &y_values = spectrum.dataY();
    std::transform(
        processingData.counts[i].cbegin(), processingData.counts[i].cend(), y_values.begin(),
        [](const std::atomic_uint32_t &val) { return static_cast<double>(val.load(std::memory_order_relaxed)); });
    auto &e_values = spectrum.dataE();
    std::transform(processingData.counts[i].cbegin(), processingData.counts[i].cend(), e_values.begin(),
                   [](const std::atomic_uint32_t &val) {
                     return std::sqrt(static_cast<double>(val.load(std::memory_order_relaxed)));
                   });
  }
}

void AlignAndFocusPowderSlim::initCalibrationConstants(API::MatrixWorkspace_sptr &wksp,
                                                       const std::vector<double> &difc_focus) {
  const auto detInfo = wksp->detectorInfo();

  for (auto iter = detInfo.cbegin(); iter != detInfo.cend(); ++iter) {
    if (!iter->isMonitor()) {
      const auto difc_focussed = getFocussedPostion(static_cast<detid_t>(iter->detid()), difc_focus, detIDToSpecNum);
      m_calibration.emplace(static_cast<detid_t>(iter->detid()),
                            difc_focussed / detInfo.difcUncalibrated(iter->index()));
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
    const auto difc_focussed = getFocussedPostion(detid, difc_focus, detIDToSpecNum);
    m_calibration.emplace(detid, difc_focussed / detc);
  }

  const MaskWorkspace_sptr maskWS = alg->getProperty("OutputMaskWorkspace");
  m_masked = maskWS->getMaskedDetectors();
  g_log.debug() << "Masked detectors: " << m_masked.size() << '\n';
}

/**
 * For fast logs, calculate the sample position correction. This is a separate implementation of
 * Mantid::API::TimeAtSampleElastic that uses DetectorInfo. Also scale by 1000 to convert from μs to ns.
 */
void AlignAndFocusPowderSlim::initScaleAtSample(const API::MatrixWorkspace_sptr &wksp) {
  // detector information for all of the L2
  const auto detInfo = wksp->detectorInfo();
  // cache a single L1 value
  const double L1 = detInfo.l1();

  if (this->getProperty(PropertyNames::CORRECTION_TO_SAMPLE)) {
    // calculate scale factors for each detector
    for (auto iter = detInfo.cbegin(); iter != detInfo.cend(); ++iter) {
      if (!iter->isMonitor()) {
        const double path_correction = L1 / (L1 + iter->l2()) * 1000.0;
        m_scale_at_sample.emplace(static_cast<detid_t>(iter->detid()), path_correction);
      }
    }
  } else {
    // set all scale factors to 1.0
    for (auto iter = detInfo.cbegin(); iter != detInfo.cend(); ++iter) {
      if (!iter->isMonitor()) {
        m_scale_at_sample.emplace(static_cast<detid_t>(iter->detid()), 1000.0);
      }
    }
  }
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

/**
 * @brief Create a TimeROI based on the filtering properties set in the algorithm. FilterByTimeStart, FilterByTimeStop
 * and FilterBadPulses
 *
 * @param wksp The workspace to get the run start time and logs from.
 * @return Kernel::TimeROI The constructed TimeROI for filtering.
 */
Kernel::TimeROI AlignAndFocusPowderSlim::getFilterROI(const API::MatrixWorkspace_sptr &wksp) {
  Kernel::TimeROI roi;
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
      // need to have centre=true for proton_charge
      roi = log->makeFilterByValue(min_pcharge, max_pcharge, true, Mantid::Kernel::TimeInterval(run_start, run_stop),
                                   0.0, true, &roi);
    }
  }
  return roi;
}

/**
 * @brief Determine the pulse indices for a given workspace and time ROI.
 *
 * @param wksp The workspace to get the pulse times from.
 * @param filterROI The time ROI to use for filtering.
 * @return std::vector<PulseROI> A vector of PulseROI representing the pulse indices to include.
 */
std::vector<PulseROI> AlignAndFocusPowderSlim::determinePulseIndices(const API::MatrixWorkspace_sptr &wksp,
                                                                     const TimeROI &filterROI) {

  std::vector<PulseROI> pulse_indices;
  if (filterROI.useAll()) {
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

    pulse_indices = filterROI.calculate_indices(*pulse_times);
    if (pulse_indices.empty())
      throw std::invalid_argument("No valid pulse time indices found for filtering");
  }

  return pulse_indices;
}

/**
 * @brief Determine the pulse indices for a given workspace, time ROI, and time splitter.
 *
 * @param wksp The workspace to get the pulse times from.
 * @param filterROI The time ROI to use for filtering.
 * @param timeSplitter The time splitter to use for determining target indices and additional time ROIs.
 * @return std::vector<std::pair<int, PulseROI>> A vector of pairs, where each pair contains a target index and a
 * PulseROI representing the pulse indices to include.
 */
std::vector<std::pair<int, PulseROI>>
AlignAndFocusPowderSlim::determinePulseIndicesTargets(const API::MatrixWorkspace_sptr &wksp, const TimeROI &filterROI,
                                                      const TimeSplitter &timeSplitter) {
  // get pulse times from frequency log on workspace
  const auto frequency_log = dynamic_cast<const TimeSeriesProperty<double> *>(wksp->run().getProperty("frequency"));
  if (!frequency_log) {
    throw std::runtime_error("Frequency log not found in workspace run");
  }
  const auto pulse_times =
      std::make_unique<std::vector<Mantid::Types::Core::DateAndTime>>(frequency_log->timesAsVector());

  std::vector<PulseROI> pulse_indices;
  if (filterROI.useAll()) {
    pulse_indices.emplace_back(0, std::numeric_limits<size_t>::max());
  } else {
    pulse_indices = filterROI.calculate_indices(*pulse_times);
    if (pulse_indices.empty())
      throw std::invalid_argument("No valid pulse time indices found for filtering");
  }

  const auto target_to_pulse_indices = timeSplitter.calculate_target_indices(*pulse_times);

  // calculate intersection of target pulse indices and time filter pulse indices (removes pulses outside filterROI)
  std::vector<std::pair<int, PulseROI>> intersected_target_pulse_indices;
  auto pulse_it = pulse_indices.cbegin();
  for (const auto &target_pair : target_to_pulse_indices) {
    // move pulse_it to the first pulse that could overlap
    while (pulse_it != pulse_indices.cend() && pulse_it->second <= target_pair.second.first) {
      ++pulse_it;
    }
    // check for overlaps
    auto check_it = pulse_it;
    while (check_it != pulse_indices.cend() && check_it->first < target_pair.second.second) {
      // there is an overlap
      size_t start_index = std::max(check_it->first, target_pair.second.first);
      size_t stop_index = std::min(check_it->second, target_pair.second.second);
      if (start_index < stop_index) {
        intersected_target_pulse_indices.emplace_back(target_pair.first, PulseROI(start_index, stop_index));
      }
      ++check_it;
    }
  }

  return intersected_target_pulse_indices;
}

TimeSplitter
AlignAndFocusPowderSlim::timeSplitterFromSplitterWorkspace(const Types::Core::DateAndTime &filterStartTime) {
  API::Workspace_sptr tempws = this->getProperty(PropertyNames::SPLITTER_WS);
  DataObjects::SplittersWorkspace_sptr splittersWorkspace =
      std::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(tempws);
  DataObjects::TableWorkspace_sptr splitterTableWorkspace =
      std::dynamic_pointer_cast<DataObjects::TableWorkspace>(tempws);
  API::MatrixWorkspace_sptr matrixSplitterWS = std::dynamic_pointer_cast<API::MatrixWorkspace>(tempws);

  if (!splittersWorkspace && !splitterTableWorkspace && !matrixSplitterWS)
    return {};

  const bool isSplittersRelativeTime = this->getProperty(PropertyNames::SPLITTER_RELATIVE);

  TimeSplitter time_splitter;
  if (splittersWorkspace) {
    time_splitter = TimeSplitter{splittersWorkspace};
  } else if (splitterTableWorkspace) {
    time_splitter =
        TimeSplitter(splitterTableWorkspace, isSplittersRelativeTime ? filterStartTime : DateAndTime::GPS_EPOCH);
  } else {
    time_splitter = TimeSplitter(matrixSplitterWS, isSplittersRelativeTime ? filterStartTime : DateAndTime::GPS_EPOCH);
  }

  return time_splitter;
}

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
