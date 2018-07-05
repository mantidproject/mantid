#include <cmath>
#include <cstdio>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadBBY.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/math/special_functions/round.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Util/PropertyFileConfiguration.h>

namespace Mantid {
namespace DataHandling {

// register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadBBY)

// consts
static const size_t HISTO_BINS_X = 240;
static const size_t HISTO_BINS_Y = 256;
// 100 = 40 + 20 + 40
static const size_t Progress_LoadBinFile = 48;
static const size_t Progress_ReserveMemory = 4;
static const size_t Progress_Total =
    2 * Progress_LoadBinFile + Progress_ReserveMemory;

static char const *const FilenameStr = "Filename";
static char const *const MaskStr = "Mask";

static char const *const FilterByTofMinStr = "FilterByTofMin";
static char const *const FilterByTofMaxStr = "FilterByTofMax";

static char const *const FilterByTimeStartStr = "FilterByTimeStart";
static char const *const FilterByTimeStopStr = "FilterByTimeStop";

using ANSTO::EventVector_pt;

template <typename TYPE>
void AddSinglePointTimeSeriesProperty(API::LogManager &logManager,
                                      const std::string &time,
                                      const std::string &name,
                                      const TYPE value) {
  // create time series property and add single value
  auto p = new Kernel::TimeSeriesProperty<TYPE>(name);
  p->addValue(time, value);

  // add to log manager
  logManager.addProperty(p);
}

/**
 * Return the confidence value that this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadBBY::confidence(Kernel::FileDescriptor &descriptor) const {
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
    if ((len > 4) &&
        (subFile.find_first_of("\\/", 0, 2) == std::string::npos)) {
      if ((subFile.rfind(".hdf") == len - 4) &&
          (subFile.compare(0, 3, "BBY") == 0))
        hdfFiles++;
      else if (subFile.rfind(".bin") == len - 4)
        binFiles++;
    }
  }

  return (hdfFiles == 1) && (binFiles == 1) ? 50 : 0;
}
/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or
 * read from after the execution (output).
 */
void LoadBBY::init() {
  // Specify file extensions which can be associated with a specific file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.emplace_back(".tar");
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      FilenameStr, "", API::FileProperty::Load, exts),
                  "The input filename of the stored data");

  // mask
  exts.clear();
  exts.emplace_back(".xml");
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      MaskStr, "", API::FileProperty::OptionalLoad, exts),
                  "The input filename of the mask data");

  // OutputWorkspace
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::IEventWorkspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output));

  // FilterByTofMin
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<double>>(
                      FilterByTofMinStr, 0, Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");

  // FilterByTofMax
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<double>>(
                      FilterByTofMaxStr, EMPTY_DBL(), Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds. Keep "
                  "blank to load all events.");

  // FilterByTimeStart
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<double>>(
          FilterByTimeStartStr, 0.0, Kernel::Direction::Input),
      "Optional: To only include events after the provided start time, in "
      "seconds (relative to the start of the run).");

  // FilterByTimeStop
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<double>>(
          FilterByTimeStopStr, EMPTY_DBL(), Kernel::Direction::Input),
      "Optional: To only include events before the provided stop time, in "
      "seconds (relative to the start of the run).");

  std::string grpOptional = "Filters";
  setPropertyGroup(FilterByTofMinStr, grpOptional);
  setPropertyGroup(FilterByTofMaxStr, grpOptional);
  setPropertyGroup(FilterByTimeStartStr, grpOptional);
  setPropertyGroup(FilterByTimeStopStr, grpOptional);
}
/**
 * Execute the algorithm.
 */
void LoadBBY::exec() {
  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (API::AnalysisDataService::Instance().doesExist(outName))
    API::AnalysisDataService::Instance().remove(outName);

  // Get the name of the data file.
  std::string filename = getPropertyValue(FilenameStr);
  ANSTO::Tar::File tarFile(filename);
  if (!tarFile.good())
    throw std::invalid_argument("invalid BBY file");

  // region of intreset
  std::vector<bool> roi = createRoiVector(getPropertyValue(MaskStr));

  double tofMinBoundary = getProperty(FilterByTofMinStr);
  double tofMaxBoundary = getProperty(FilterByTofMaxStr);

  double timeMinBoundary = getProperty(FilterByTimeStartStr);
  double timeMaxBoundary = getProperty(FilterByTimeStopStr);

  if (isEmpty(tofMaxBoundary))
    tofMaxBoundary = std::numeric_limits<double>::infinity();
  if (isEmpty(timeMaxBoundary))
    timeMaxBoundary = std::numeric_limits<double>::infinity();

  API::Progress prog(this, 0.0, 1.0, Progress_Total);
  prog.doReport("creating instrument");

  // create instrument
  InstrumentInfo instrumentInfo;

  createInstrument(tarFile, /* ref */ instrumentInfo);

  // create workspace
  DataObjects::EventWorkspace_sptr eventWS =
      boost::make_shared<DataObjects::EventWorkspace>();

  eventWS->initialize(HISTO_BINS_Y * HISTO_BINS_X,
                      2, // number of TOF bin boundaries
                      1);

  // set the units
  if (instrumentInfo.is_tof)
    eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  else
    eventWS->getAxis(0)->unit() =
        Kernel::UnitFactory::Instance().create("Wavelength");

  eventWS->setYUnit("Counts");

  // set title
  const std::vector<std::string> &subFiles = tarFile.files();
  for (const auto &subFile : subFiles)
    if (subFile.compare(0, 3, "BBY") == 0) {
      std::string title = subFile;

      if (title.rfind(".hdf") == title.length() - 4)
        title.resize(title.length() - 4);

      if (title.rfind(".nx") == title.length() - 3)
        title.resize(title.length() - 3);

      eventWS->setTitle(title);
      break;
    }

  // load events
  size_t numberHistograms = eventWS->getNumberHistograms();

  std::vector<EventVector_pt> eventVectors(numberHistograms, nullptr);
  std::vector<size_t> eventCounts(numberHistograms, 0);

  // phase correction

  double periodMaster = instrumentInfo.period_master;
  double periodSlave = instrumentInfo.period_slave;
  double phaseSlave = instrumentInfo.phase_slave;

  double period = periodSlave;
  double shift = -1.0 / 6.0 * periodMaster - periodSlave * phaseSlave / 360.0;

  // count total events per pixel to reserve necessary memory
  ANSTO::EventCounter eventCounter(
      roi, HISTO_BINS_Y, period, shift, tofMinBoundary, tofMaxBoundary,
      timeMinBoundary, timeMaxBoundary, eventCounts);

  loadEvents(prog, "loading neutron counts", tarFile, eventCounter);

  // prepare event storage
  ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists",
                                     numberHistograms, Progress_ReserveMemory);

  for (size_t i = 0; i != numberHistograms; ++i) {
    DataObjects::EventList &eventList = eventWS->getSpectrum(i);

    eventList.setSortOrder(DataObjects::PULSETIME_SORT);
    eventList.reserve(eventCounts[i]);

    eventList.setDetectorID(static_cast<detid_t>(i));
    eventList.setSpectrumNo(static_cast<detid_t>(i));

    DataObjects::getEventsFrom(eventList, eventVectors[i]);

    progTracker.update(i);
  }
  progTracker.complete();

  if (instrumentInfo.is_tof) {
    ANSTO::EventAssigner eventAssigner(
        roi, HISTO_BINS_Y, period, shift, tofMinBoundary, tofMaxBoundary,
        timeMinBoundary, timeMaxBoundary, eventVectors);

    loadEvents(prog, "loading neutron events (TOF)", tarFile, eventAssigner);
  } else {
    ANSTO::EventAssignerFixedWavelength eventAssigner(
        roi, HISTO_BINS_Y, instrumentInfo.wavelength, period, shift,
        tofMinBoundary, tofMaxBoundary, timeMinBoundary, timeMaxBoundary,
        eventVectors);

    loadEvents(prog, "loading neutron events (Wavelength)", tarFile,
               eventAssigner);
  }

  if (instrumentInfo.is_tof) {
    // just to make sure the bins hold it all
    eventWS->setAllX(
        HistogramData::BinEdges{std::max(0.0, floor(eventCounter.tofMin())),
                                eventCounter.tofMax() + 1});
  } else {
    // +/-10%
    eventWS->setAllX(HistogramData::BinEdges{instrumentInfo.wavelength * 0.9,
                                             instrumentInfo.wavelength * 1.1});
  }

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

    API::IAlgorithm_sptr maskingAlg = createChildAlgorithm("MaskDetectors");
    maskingAlg->setProperty("Workspace", eventWS);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }

  // set log values
  API::LogManager &logManager = eventWS->mutableRun();

  int frame_count = static_cast<int>(eventCounter.numFrames());

  logManager.addProperty("filename", filename);
  logManager.addProperty("att_pos", static_cast<int>(instrumentInfo.att_pos));
  logManager.addProperty("frame_count", frame_count);
  logManager.addProperty("period", period);

  // currently beam monitor counts are not available, instead number of frames
  // times period is used
  logManager.addProperty(
      "bm_counts", static_cast<double>(frame_count) * period /
                       1.0e6); // static_cast<double>(instrumentInfo.bm_counts)

  Types::Core::time_duration duration = boost::posix_time::microseconds(
      static_cast<boost::int64_t>(static_cast<double>(frame_count) * period));

  Types::Core::DateAndTime start_time("2000-01-01T00:00:00");
  Types::Core::DateAndTime end_time(start_time + duration);

  logManager.addProperty("start_time", start_time.toISO8601String());
  logManager.addProperty("end_time", end_time.toISO8601String());
  logManager.addProperty("is_tof", instrumentInfo.is_tof);

  std::string time_str = start_time.toISO8601String();

  AddSinglePointTimeSeriesProperty(logManager, time_str, "sample_name",
                                   instrumentInfo.sample_name);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "sample_description",
                                   instrumentInfo.sample_description);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "sample_aperture",
                                   instrumentInfo.sample_aperture);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "sample_x",
                                   instrumentInfo.sample_x);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "sample_y",
                                   instrumentInfo.sample_y);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "sample_z",
                                   instrumentInfo.sample_z);

  AddSinglePointTimeSeriesProperty(logManager, time_str, "wavelength",
                                   instrumentInfo.wavelength);

  AddSinglePointTimeSeriesProperty(logManager, time_str, "source_aperture",
                                   instrumentInfo.source_aperture);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "master1_chopper_id",
                                   instrumentInfo.master1_chopper_id);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "master2_chopper_id",
                                   instrumentInfo.master2_chopper_id);

  AddSinglePointTimeSeriesProperty(logManager, time_str, "Lt0_value",
                                   instrumentInfo.Lt0_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "Ltof_curtainl_value",
                                   instrumentInfo.Ltof_curtainl_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "Ltof_curtainr_value",
                                   instrumentInfo.Ltof_curtainr_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "Ltof_curtainu_value",
                                   instrumentInfo.Ltof_curtainu_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "Ltof_curtaind_value",
                                   instrumentInfo.Ltof_curtaind_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L1_chopper_value",
                                   instrumentInfo.L1_chopper_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L1",
                                   instrumentInfo.L1_source_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_det_value",
                                   instrumentInfo.L2_det_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtainl_value",
                                   instrumentInfo.L2_curtainl_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtainr_value",
                                   instrumentInfo.L2_curtainr_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtainu_value",
                                   instrumentInfo.L2_curtainu_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtaind_value",
                                   instrumentInfo.L2_curtaind_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtainl_value",
                                   instrumentInfo.D_curtainl_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtainr_value",
                                   instrumentInfo.D_curtainr_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtainu_value",
                                   instrumentInfo.D_curtainu_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtaind_value",
                                   instrumentInfo.D_curtaind_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "curtain_rotation",
                                   10.0);

  API::IAlgorithm_sptr loadInstrumentAlg =
      createChildAlgorithm("LoadInstrument");
  loadInstrumentAlg->setProperty("Workspace", eventWS);
  loadInstrumentAlg->setPropertyValue("InstrumentName", "BILBY");
  loadInstrumentAlg->setProperty("RewriteSpectraMap",
                                 Mantid::Kernel::OptionalBool(false));
  loadInstrumentAlg->executeAsChildAlg();

  setProperty("OutputWorkspace", eventWS);
}

// region of intreset
std::vector<bool> LoadBBY::createRoiVector(const std::string &maskfile) {
  std::vector<bool> result(HISTO_BINS_Y * HISTO_BINS_X, true);

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
      std::stringstream ss(line);

      std::string item;
      while (std::getline(ss, item, ',')) {
        auto k = item.find('-');

        size_t p0, p1;
        if (k != std::string::npos) {
          p0 = boost::lexical_cast<size_t>(item.substr(0, k));
          p1 = boost::lexical_cast<size_t>(
              item.substr(k + 1, item.size() - k - 1));

          if (p0 > p1)
            std::swap(p0, p1);
        } else {
          p0 = boost::lexical_cast<size_t>(item);
          p1 = p0;
        }

        if (p0 < result.size()) {
          if (p1 >= result.size())
            p1 = result.size() - 1;

          while (p0 <= p1)
            result[p0++] = false;
        }
      }
    }
  }

  return result;
}

// instrument creation
void LoadBBY::createInstrument(ANSTO::Tar::File &tarFile,
                               InstrumentInfo &instrumentInfo) {

  const double toMeters = 1.0 / 1000;

  instrumentInfo.bm_counts = 0;
  instrumentInfo.att_pos = 0;
  instrumentInfo.is_tof = true;
  instrumentInfo.wavelength = 0.0;

  instrumentInfo.sample_name = "UNKNOWN";
  instrumentInfo.sample_description = "UNKNOWN";
  instrumentInfo.sample_aperture = 0.0;
  instrumentInfo.sample_x = 0.0;
  instrumentInfo.sample_y = 0.0;
  instrumentInfo.sample_z = 0.0;

  instrumentInfo.source_aperture = 0.0;
  instrumentInfo.master1_chopper_id = -1;
  instrumentInfo.master2_chopper_id = -1;

  instrumentInfo.period_master = 0.0;
  instrumentInfo.period_slave = (1.0 / 50.0) * 1.0e6;
  instrumentInfo.phase_slave = 0.0;

  instrumentInfo.Lt0_value = 0.0;
  instrumentInfo.Ltof_curtainl_value = 0.0;
  instrumentInfo.Ltof_curtainr_value = 0.0;
  instrumentInfo.Ltof_curtainu_value = 0.0;
  instrumentInfo.Ltof_curtaind_value = 0.0;

  instrumentInfo.L1_chopper_value = 18.47258984375;
  instrumentInfo.L1_source_value = 16.671;
  instrumentInfo.L2_det_value = 33.15616015625;

  instrumentInfo.L2_curtainl_value = 23.28446093750;
  instrumentInfo.L2_curtainr_value = 23.28201953125;
  instrumentInfo.L2_curtainu_value = 24.28616015625;
  instrumentInfo.L2_curtaind_value = 24.28235937500;

  instrumentInfo.D_curtainl_value = 0.3816;
  instrumentInfo.D_curtainr_value = 0.4024;
  instrumentInfo.D_curtainu_value = 0.3947;
  instrumentInfo.D_curtaind_value = 0.3978;

  // extract log and hdf file
  const std::vector<std::string> &files = tarFile.files();
  auto file_it =
      std::find_if(files.cbegin(), files.cend(), [](const std::string &file) {
        return file.rfind(".hdf") == file.length() - 4;
      });
  if (file_it != files.end()) {
    tarFile.select(file_it->c_str());
    // extract hdf file into tmp file
    Poco::TemporaryFile hdfFile;
    boost::shared_ptr<FILE> handle(fopen(hdfFile.path().c_str(), "wb"), fclose);
    if (handle) {
      // copy content
      char buffer[4096];
      size_t bytesRead;
      while (0 != (bytesRead = tarFile.read(buffer, sizeof(buffer))))
        fwrite(buffer, bytesRead, 1, handle.get());
      handle.reset();

      NeXus::NXRoot root(hdfFile.path());
      NeXus::NXEntry entry = root.openFirstEntry();

      float tmp_float = 0.0f;
      int32_t tmp_int32 = 0;
      std::string tmp_str;

      if (loadNXDataSet(entry, "monitor/bm1_counts", tmp_int32))
        instrumentInfo.bm_counts = tmp_int32;
      if (loadNXDataSet(entry, "instrument/att_pos", tmp_float))
        instrumentInfo.att_pos =
            boost::math::iround(tmp_float); // [1.0, 2.0, ..., 5.0]

      if (loadNXString(entry, "sample/name", tmp_str))
        instrumentInfo.sample_name = tmp_str;
      if (loadNXString(entry, "sample/description", tmp_str))
        instrumentInfo.sample_description = tmp_str;
      if (loadNXDataSet(entry, "sample/sample_aperture", tmp_float))
        instrumentInfo.sample_aperture = tmp_float;
      if (loadNXDataSet(entry, "sample/samx", tmp_float))
        instrumentInfo.sample_x = tmp_float;
      if (loadNXDataSet(entry, "sample/samy", tmp_float))
        instrumentInfo.sample_y = tmp_float;
      if (loadNXDataSet(entry, "sample/samz", tmp_float))
        instrumentInfo.sample_z = tmp_float;

      if (loadNXDataSet(entry, "instrument/source_aperture", tmp_float))
        instrumentInfo.source_aperture = tmp_float;
      if (loadNXDataSet(entry, "instrument/master1_chopper_id", tmp_int32))
        instrumentInfo.master1_chopper_id = tmp_int32;
      if (loadNXDataSet(entry, "instrument/master2_chopper_id", tmp_int32))
        instrumentInfo.master2_chopper_id = tmp_int32;

      if (loadNXString(entry, "instrument/detector/frame_source", tmp_str))
        instrumentInfo.is_tof = tmp_str == "EXTERNAL";
      if (loadNXDataSet(entry, "instrument/nvs067/lambda", tmp_float))
        instrumentInfo.wavelength = tmp_float;

      if (loadNXDataSet(entry, "instrument/master_chopper_freq", tmp_float) &&
          (tmp_float > 0.0f))
        instrumentInfo.period_master = 1.0 / tmp_float * 1.0e6;
      if (loadNXDataSet(entry, "instrument/t0_chopper_freq", tmp_float) &&
          (tmp_float > 0.0f))
        instrumentInfo.period_slave = 1.0 / tmp_float * 1.0e6;
      if (loadNXDataSet(entry, "instrument/t0_chopper_phase", tmp_float))
        instrumentInfo.phase_slave = tmp_float < 999.0 ? tmp_float : 0.0;

      if (loadNXDataSet(entry, "instrument/Lt0", tmp_float))
        instrumentInfo.Lt0_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/Ltof_curtainl", tmp_float))
        instrumentInfo.Ltof_curtainl_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/Ltof_curtainr", tmp_float))
        instrumentInfo.Ltof_curtainr_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/Ltof_curtainu", tmp_float))
        instrumentInfo.Ltof_curtainu_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/Ltof_curtaind", tmp_float))
        instrumentInfo.Ltof_curtaind_value = tmp_float * toMeters;

      if (loadNXDataSet(entry, "instrument/L2_det", tmp_float))
        instrumentInfo.L2_det_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/Ltof_det", tmp_float))
        instrumentInfo.L1_chopper_value =
            tmp_float * toMeters - instrumentInfo.L2_det_value;
      if (loadNXDataSet(entry, "instrument/L1", tmp_float))
        instrumentInfo.L1_source_value = tmp_float * toMeters;

      if (loadNXDataSet(entry, "instrument/L2_curtainl", tmp_float))
        instrumentInfo.L2_curtainl_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/L2_curtainr", tmp_float))
        instrumentInfo.L2_curtainr_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/L2_curtainu", tmp_float))
        instrumentInfo.L2_curtainu_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/L2_curtaind", tmp_float))
        instrumentInfo.L2_curtaind_value = tmp_float * toMeters;

      if (loadNXDataSet(entry, "instrument/detector/curtainl", tmp_float))
        instrumentInfo.D_curtainl_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/detector/curtainr", tmp_float))
        instrumentInfo.D_curtainr_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/detector/curtainu", tmp_float))
        instrumentInfo.D_curtainu_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/detector/curtaind", tmp_float))
        instrumentInfo.D_curtaind_value = tmp_float * toMeters;
    }
  }

  // patching
  file_it = std::find(files.cbegin(), files.cend(), "History.log");
  if (file_it != files.cend()) {
    tarFile.select(file_it->c_str());
    std::string logContent;
    logContent.resize(tarFile.selected_size());
    tarFile.read(&logContent[0], logContent.size());
    std::istringstream data(logContent);
    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> conf(
        new Poco::Util::PropertyFileConfiguration(data));

    if (conf->hasProperty("Bm1Counts"))
      instrumentInfo.bm_counts = conf->getInt("Bm1Counts");
    if (conf->hasProperty("AttPos"))
      instrumentInfo.att_pos = boost::math::iround(conf->getDouble("AttPos"));

    if (conf->hasProperty("SampleName"))
      instrumentInfo.sample_name = conf->getString("SampleName");
    if (conf->hasProperty("SampleAperture"))
      instrumentInfo.sample_aperture = conf->getDouble("SampleAperture");
    if (conf->hasProperty("SourceAperture"))
      instrumentInfo.source_aperture = conf->getDouble("SourceAperture");

    if (conf->hasProperty("MasterChopperFreq")) {
      auto tmp = conf->getDouble("MasterChopperFreq");
      if (tmp > 0.0f)
        instrumentInfo.period_master = 1.0 / tmp * 1.0e6;
    }
    if (conf->hasProperty("T0ChopperFreq")) {
      auto tmp = conf->getDouble("T0ChopperFreq");
      if (tmp > 0.0f)
        instrumentInfo.period_slave = 1.0 / tmp * 1.0e6;
    }
    if (conf->hasProperty("T0ChopperPhase")) {
      auto tmp = conf->getDouble("T0ChopperPhase");
      instrumentInfo.phase_slave = tmp < 999.0 ? tmp : 0.0;
    }

    if (conf->hasProperty("FrameSource"))
      instrumentInfo.is_tof = conf->getString("FrameSource") == "EXTERNAL";
    if (conf->hasProperty("Wavelength"))
      instrumentInfo.wavelength = conf->getDouble("Wavelength");

    if (conf->hasProperty("L1"))
      instrumentInfo.L1_source_value = conf->getDouble("L1") * toMeters;
    if (conf->hasProperty("LTofDet"))
      instrumentInfo.L1_chopper_value =
          conf->getDouble("LTofDet") * toMeters - instrumentInfo.L2_det_value;
    if (conf->hasProperty("L2Det"))
      instrumentInfo.L2_det_value = conf->getDouble("L2Det") * toMeters;

    if (conf->hasProperty("L2CurtainL"))
      instrumentInfo.L2_curtainl_value =
          conf->getDouble("L2CurtainL") * toMeters;
    if (conf->hasProperty("L2CurtainR"))
      instrumentInfo.L2_curtainr_value =
          conf->getDouble("L2CurtainR") * toMeters;
    if (conf->hasProperty("L2CurtainU"))
      instrumentInfo.L2_curtainu_value =
          conf->getDouble("L2CurtainU") * toMeters;
    if (conf->hasProperty("L2CurtainD"))
      instrumentInfo.L2_curtaind_value =
          conf->getDouble("L2CurtainD") * toMeters;

    if (conf->hasProperty("CurtainL"))
      instrumentInfo.D_curtainl_value = conf->getDouble("CurtainL") * toMeters;
    if (conf->hasProperty("CurtainR"))
      instrumentInfo.D_curtainr_value = conf->getDouble("CurtainR") * toMeters;
    if (conf->hasProperty("CurtainU"))
      instrumentInfo.D_curtainu_value = conf->getDouble("CurtainU") * toMeters;
    if (conf->hasProperty("CurtainD"))
      instrumentInfo.D_curtaind_value = conf->getDouble("CurtainD") * toMeters;
  }
}

// load nx dataset
template <class T>
bool LoadBBY::loadNXDataSet(NeXus::NXEntry &entry, const std::string &path,
                            T &value) {
  try {
    NeXus::NXDataSetTyped<T> dataSet = entry.openNXDataSet<T>(path);
    dataSet.load();

    value = *dataSet();
    return true;
  } catch (std::runtime_error &) {
    return false;
  }
}
bool LoadBBY::loadNXString(NeXus::NXEntry &entry, const std::string &path,
                           std::string &value) {
  try {
    NeXus::NXChar dataSet = entry.openNXChar(path);
    dataSet.load();

    value = std::string(dataSet(), dataSet.dim0());
    return true;
  } catch (std::runtime_error &) {
    return false;
  }
}

// read counts/events from binary file
template <class EventProcessor>
void LoadBBY::loadEvents(API::Progress &prog, const char *progMsg,
                         ANSTO::Tar::File &tarFile,
                         EventProcessor &eventProcessor) {
  prog.doReport(progMsg);

  // select bin file
  int64_t fileSize = 0;
  const std::vector<std::string> &files = tarFile.files();
  for (const auto &file : files)
    if (file.rfind(".bin") == file.length() - 4) {
      tarFile.select(file.c_str());
      fileSize = tarFile.selected_size();
      break;
    }

  // for progress notifications
  ANSTO::ProgressTracker progTracker(prog, progMsg, fileSize,
                                     Progress_LoadBinFile);

  uint32_t x = 0; // 9 bits [0-239] tube number
  uint32_t y = 0; // 8 bits [0-255] position along tube

  // uint v = 0; // 0 bits [     ]
  // uint w = 0; // 0 bits [     ] energy
  uint32_t dt = 0;
  double tof = 0.0;

  if ((fileSize == 0) || !tarFile.skip(128))
    return;

  int state = 0;
  uint32_t c;
  while ((c = static_cast<uint32_t>(tarFile.read_byte())) !=
         static_cast<uint32_t>(-1)) {

    bool event_ended = false;
    switch (state) {
    case 0:
      x = (c & 0xFF) >> 0; // set bit 1-8
      break;

    case 1:
      x |= (c & 0x01) << 8; // set bit 9
      y = (c & 0xFE) >> 1;  // set bit 1-7
      break;

    case 2:
      event_ended = (c & 0xC0) != 0xC0;
      if (!event_ended)
        c &= 0x3F;

      y |= (c & 0x01) << 7; // set bit 8
      dt = (c & 0xFE) >> 1; // set bit 1-5(7)
      break;

    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      event_ended = (c & 0xC0) != 0xC0;
      if (!event_ended)
        c &= 0x3F;

      // state is either 3, 4, 5, 6 or 7
      dt |= (c & 0xFF) << (5 + 6 * (state - 3)); // set bit 6...
      break;
    }
    state++;

    if (event_ended || (state == 8)) {
      state = 0;

      if ((x == 0) && (y == 0) && (dt == 0xFFFFFFFF)) {
        tof = 0.0;
        eventProcessor.newFrame();
      } else if ((x >= HISTO_BINS_X) || (y >= HISTO_BINS_Y)) {
        // ignore
      } else {
        // conversion from 100 nanoseconds to 1 microsecond
        tof += static_cast<int>(dt) * 0.1;

        eventProcessor.addEvent(x, y, tof);
      }

      progTracker.update(tarFile.selected_position());
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
