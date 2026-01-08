// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadANSTOHelper.h"
#include "MantidDataHandling/LoadBBY2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/math/special_functions/round.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Util/PropertyFileConfiguration.h>

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace Nexus;

// register the algorithm
DECLARE_NEXUS_LAZY_FILELOADER_ALGORITHM(LoadBBY2)

// consts
static const int LAST_INDEX = -1;
static const size_t HISTO_BINS_X = 240;
static const size_t HISTO_BINS_Y = 256;
// 100 = 40 + 20 + 40
static const size_t Progress_LoadBinFile = 48;
static const size_t Progress_ReserveMemory = 4;
static const size_t Progress_Total = 2 * Progress_LoadBinFile + Progress_ReserveMemory;

static char const *const FilenameStr = "Filename";
static char const *const MaskStr = "Mask";

static char const *const FilterByTofMinStr = "FilterByTofMin";
static char const *const FilterByTofMaxStr = "FilterByTofMax";

static char const *const FilterByTimeStartStr = "FilterByTimeStart";
static char const *const FilterByTimeStopStr = "FilterByTimeStop";

static char const *const UseHMScanTimeStr = "UseHMScanTime";

using namespace ANSTO;
using ANSTO::EventVector_pt;

static const std::map<std::string, Anxs::ScanLog> ScanLogMap = {
    {"end", Anxs::ScanLog::End}, {"mean", Anxs::ScanLog::Mean}, {"start", Anxs::ScanLog::Start}};

template <typename T>
void traceStatistics(const Nexus::NXEntry &entry, const std::string &path, uint64_t startTime, uint64_t endTime,
                     Kernel::Logger &log) {

  if (log.isDebug()) {

    std::vector<uint64_t> times;
    std::vector<T> values;
    std::string units;
    auto n = Anxs::extractTimedDataSet<T>(entry, path, startTime, endTime, times, values, units);

    // log stats on the parameter variation
    if (n > 0) {
      auto meanX = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<T>(n);
      T accum{0};
      std::for_each(values.begin(), values.end(), [&](const double d) { accum += (d - meanX) * (d - meanX); });
      auto stdX = sqrt(accum / static_cast<T>(n));
      auto result = std::minmax_element(values.begin(), values.end());
      log.debug() << "Log parameter " << path << ": " << meanX << " +- " << stdX << ", " << *result.first << " ... "
                  << *result.second << ", pts " << n << std::endl;
    } else {
      log.debug() << "Cannot find : " << path << std::endl;
      return;
    }
  }
}

template <typename TYPE>
void addSinglePointTimeSeriesProperty(API::LogManager &logManager, const std::string &time, const std::string &name,
                                      const TYPE value) {
  // create time series property and add single value
  auto p = new Kernel::TimeSeriesProperty<TYPE>(name);
  p->addValue(time, value);

  // add to log manager
  logManager.addProperty(p);
}

template <typename EP>
void loadEvents(API::Progress &prog, const char *progMsg, EP &eventProcessor, const Nexus::NXEntry &entry,
                uint64_t start_nsec, uint64_t end_nsec) {

  using namespace ANSTO;

  prog.doReport(progMsg);

  // for progress notifications
  ANSTO::ProgressTracker progTracker(prog, progMsg, Progress_LoadBinFile, Progress_LoadBinFile);

  const std::string neutronPath{"instrument/detector_events"};
  Anxs::ReadEventData(progTracker, entry, &eventProcessor, start_nsec, end_nsec, neutronPath, HISTO_BINS_Y);
}

/// Empty default constructor
LoadBBY2::LoadBBY2() {}

/**
 * Return the confidence value that this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadBBY2::confidence(Nexus::NexusDescriptorLazy &descriptor) const {

  static const std::set<std::string> requiredEntries = {"/entry1/program_name",
                                                        "/entry1/experiment/gumtree_version",
                                                        "/entry1/instrument/detector_events/event_time_zero",
                                                        "/entry1/instrument/detector_events/event_id",
                                                        "/entry1/instrument/L1/value",
                                                        "/entry1/instrument/L2_curtaind/value",
                                                        "/entry1/instrument/L2_curtainl/value",
                                                        "/entry1/instrument/L2_curtainr/value",
                                                        "/entry1/instrument/L2_curtainu/value",
                                                        "/entry1/instrument/nvs067/lambda/value",
                                                        "/entry1/instrument/shutters/fast_shutter",
                                                        "/entry1/scan_dataset/time",
                                                        "/entry1/scan_dataset/value"};

  if (std::all_of(requiredEntries.cbegin(), requiredEntries.cend(),
                  [&descriptor](const std::string &entry) { return descriptor.isEntry(entry); })) {
    return 95;
  } else {
    return 0;
  }
}
/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or
 * read from after the execution (output).
 */
void LoadBBY2::init() {
  // Specify file extensions which can be associated with a specific file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.emplace_back(".nxs");
  declareProperty(std::make_unique<API::FileProperty>(FilenameStr, "", API::FileProperty::Load, exts),
                  "The input filename of the stored data");

  // mask
  exts.clear();
  exts.emplace_back(".xml");
  declareProperty(std::make_unique<API::FileProperty>(MaskStr, "", API::FileProperty::OptionalLoad, exts),
                  "The input filename of the mask data");

  // OutputWorkspace
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::IEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output));

  // FilterByTofMin
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>(FilterByTofMinStr, 0, Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");

  // FilterByTofMax
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<double>>(FilterByTofMaxStr, EMPTY_DBL(), Kernel::Direction::Input),
      "Optional: To exclude events that do not fall within a range "
      "of times-of-flight. "
      "This is the maximum accepted value in microseconds. Keep "
      "blank to load all events.");

  // FilterByTimeStart
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<double>>(FilterByTimeStartStr, 0.0, Kernel::Direction::Input),
      "Optional: To only include events after the provided start time, in "
      "seconds (relative to the start of the run).");

  // FilterByTimeStop
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<double>>(FilterByTimeStopStr, EMPTY_DBL(), Kernel::Direction::Input),
      "Optional: To only include events before the provided stop time, in "
      "seconds (relative to the start of the run).");

  declareProperty(UseHMScanTimeStr, true, "Use hmscan time rather than scan_dataset.");

  std::string grpOptional = "Filters";
  setPropertyGroup(FilterByTofMinStr, grpOptional);
  setPropertyGroup(FilterByTofMaxStr, grpOptional);
  setPropertyGroup(FilterByTimeStartStr, grpOptional);
  setPropertyGroup(FilterByTimeStopStr, grpOptional);
}
/**
 * Execute the algorithm.
 */
void LoadBBY2::exec() {

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (API::AnalysisDataService::Instance().doesExist(outName))
    API::AnalysisDataService::Instance().remove(outName);

  // Get the name of the data file.
  std::string nxsFile = getPropertyValue(FilenameStr);

  useHMScanTime = getProperty(UseHMScanTimeStr);

  // get the root entry and time period
  Nexus::NXRoot root(nxsFile);
  Nexus::NXEntry nxsEntry = root.openFirstEntry();
  uint64_t startTime, endTime;
  if (useHMScanTime)
    std::tie(startTime, endTime) = Anxs::getHMScanLimits(nxsEntry, 0);
  else
    std::tie(startTime, endTime) = Anxs::getTimeScanLimits(nxsEntry, 0);
  if (startTime >= endTime) {
    g_log.error() << "Invalid time window from " << (useHMScanTime ? "hmscan" : "scan_dataset") << "\n";
    throw std::runtime_error("LoadBBY2: invalid or missing scan time range.");
  }

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

  // create workspace
  DataObjects::EventWorkspace_sptr eventWS = std::make_shared<DataObjects::EventWorkspace>();

  eventWS->initialize(HISTO_BINS_Y * HISTO_BINS_X,
                      2, // number of TOF bin boundaries
                      1);

  // create instrument
  InstrumentInfo instrumentInfo;
  std::map<std::string, double> logParams;
  std::map<std::string, std::string> logStrings;
  std::map<std::string, std::string> allParams;
  createInstrument(nxsEntry, startTime, endTime, instrumentInfo, logParams, logStrings, allParams);

  // set the units
  if (instrumentInfo.is_tof)
    eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  else
    eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Wavelength");

  eventWS->setYUnit("Counts");
  eventWS->setTitle(Anxs::extractWorkspaceTitle(nxsFile));

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

  // get the start time from the file
  Types::Core::DateAndTime startDateTime(instrumentInfo.start_time);
  auto startInNanosec = startDateTime.totalNanoseconds();

  // count total events per pixel to reserve necessary memory
  ANSTO::EventCounter eventCounter(roi, HISTO_BINS_Y, period, shift, startInNanosec, tofMinBoundary, tofMaxBoundary,
                                   timeMinBoundary, timeMaxBoundary, eventCounts);

  loadEvents<ANSTO::EventCounter>(prog, "loading neutron counts", eventCounter, nxsEntry, startTime, endTime);

  // prepare event storage
  ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists", numberHistograms, Progress_ReserveMemory);

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
    ANSTO::EventAssigner eventAssigner(roi, HISTO_BINS_Y, period, shift, startInNanosec, tofMinBoundary, tofMaxBoundary,
                                       timeMinBoundary, timeMaxBoundary, eventVectors);

    loadEvents(prog, "loading neutron events (TOF)", eventAssigner, nxsEntry, startTime, endTime);
  } else {
    ANSTO::EventAssignerFixedWavelength eventAssigner(roi, HISTO_BINS_Y, instrumentInfo.wavelength, period, shift,
                                                      startInNanosec, tofMinBoundary, tofMaxBoundary, timeMinBoundary,
                                                      timeMaxBoundary, eventVectors);

    loadEvents(prog, "loading neutron events (Wavelength)", eventAssigner, nxsEntry, startTime, endTime);
  }

  auto getParam = [&allParams](const std::string &tag, double defValue) {
    try {
      return std::stod(allParams[tag]);
    } catch (const std::invalid_argument &) {
      return defValue;
    }
  };
  if (instrumentInfo.is_tof) {
    // just to make sure the bins hold it all
    eventWS->setAllX(HistogramData::BinEdges{std::max(0.0, floor(eventCounter.tofMin())), eventCounter.tofMax() + 1});
  } else {
    double lof = getParam("wavelength_extn_lo", 0.95);
    double hif = getParam("wavelength_extn_hi", 1.05);
    eventWS->setAllX(HistogramData::BinEdges{instrumentInfo.wavelength * lof, instrumentInfo.wavelength * hif});
  }

  // count total number of masked bins
  size_t maskedBins = std::count_if(roi.begin(), roi.end(), [](bool v) { return !v; });

  if (maskedBins > 0) {
    // create list of masked bins
    std::vector<size_t> maskIndexList(maskedBins);
    size_t maskIndex = 0;

    for (size_t i = 0; i != roi.size(); i++)
      if (!roi[i])
        maskIndexList[maskIndex++] = i;

    auto maskingAlg = createChildAlgorithm("MaskDetectors");
    maskingAlg->setProperty("Workspace", eventWS);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }

  // set log values
  API::LogManager &logManager = eventWS->mutableRun();

  auto frame_count = static_cast<int>(eventCounter.numFrames());

  logManager.addProperty("filename", nxsFile);
  logManager.addProperty("att_pos", static_cast<int>(instrumentInfo.att_pos));
  logManager.addProperty("frame_count", frame_count);
  logManager.addProperty("period", period);

  // currently beam monitor counts are not available, instead number of frames
  // times period is used
  logManager.addProperty("bm_counts", static_cast<double>(frame_count) * period /
                                          1.0e6); // static_cast<double>(instrumentInfo.bm_counts)

  Types::Core::time_duration duration =
      boost::posix_time::microseconds(static_cast<boost::int64_t>(static_cast<double>(frame_count) * period));

  Types::Core::DateAndTime start_time(instrumentInfo.start_time);
  Types::Core::DateAndTime end_time(start_time + duration);

  logManager.addProperty("start_time", start_time.toISO8601String());
  logManager.addProperty("run_start", start_time.toISO8601String());
  logManager.addProperty("end_time", end_time.toISO8601String());
  logManager.addProperty("is_tof", instrumentInfo.is_tof);

  std::string time_str = start_time.toISO8601String();

  logManager.addProperty("sample_name", instrumentInfo.sample_name);
  logManager.addProperty("sample_description", instrumentInfo.sample_description);
  addSinglePointTimeSeriesProperty(logManager, time_str, "wavelength", instrumentInfo.wavelength);
  addSinglePointTimeSeriesProperty(logManager, time_str, "master1_chopper_id", instrumentInfo.master1_chopper_id);
  addSinglePointTimeSeriesProperty(logManager, time_str, "master2_chopper_id", instrumentInfo.master2_chopper_id);

  for (auto &x : logStrings) {
    logManager.addProperty(x.first, x.second);
  }
  for (auto &x : logParams) {
    addSinglePointTimeSeriesProperty(logManager, time_str, x.first, x.second);
  }

  auto loadInstrumentAlg = createChildAlgorithm("LoadInstrument");
  loadInstrumentAlg->setProperty("Workspace", eventWS);
  loadInstrumentAlg->setPropertyValue("InstrumentName", "BILBY");
  loadInstrumentAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loadInstrumentAlg->executeAsChildAlg();

  setProperty("OutputWorkspace", eventWS);
}

// region of intreset
std::vector<bool> LoadBBY2::createRoiVector(const std::string &maskfile) {
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
          p1 = boost::lexical_cast<size_t>(item.substr(k + 1, item.size() - k - 1));

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

// loading instrument parameters
void LoadBBY2::loadInstrumentParameters(const Nexus::NXEntry &entry, uint64_t startTime, uint64_t endTime,
                                        std::map<std::string, double> &logParams,
                                        std::map<std::string, std::string> &logStrings,
                                        std::map<std::string, std::string> &allParams) {
  using namespace Poco::XML;
  std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");

  try {
    std::string parameterFilename = idfDirectory + "BILBY_Parameters.xml";
    // Set up the DOM parser and parse xml file
    DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc;
    try {
      pDoc = pParser.parse(parameterFilename);
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse File:", parameterFilename);
    }
    NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
    Node *pNode = it.nextNode();
    while (pNode) {
      if (pNode->nodeName() == "parameter") {
        auto pElem = dynamic_cast<Element *>(pNode);
        std::string paramName = pElem->getAttribute("name");
        Poco::AutoPtr<NodeList> nodeList = pElem->childNodes();
        for (unsigned long i = 0; i < nodeList->length(); i++) {
          auto cNode = nodeList->item(i);
          if (cNode->nodeName() == "value") {
            auto cElem = dynamic_cast<Poco::XML::Element *>(cNode);
            std::string value = cElem->getAttribute("val");
            allParams[paramName] = value;
          }
        }
      }
      pNode = it.nextNode();
    }

    auto isNumeric = [](const std::string &tag) {
      try {
        auto stag = boost::algorithm::trim_copy(tag);
        size_t sz = 0;
        auto value = std::stod(stag, &sz);
        return sz > 0 && stag.size() == sz && std::isfinite(value);
      } catch (const std::invalid_argument &) {
        return false;
      }
    };

    std::string tmpString;
    double tmpDouble = 0.0f;
    uint64_t tmpTimestamp = 0;
    for (auto &x : allParams) {
      if (x.first.find("log_") == 0) {
        auto logTag = boost::algorithm::trim_copy(x.first.substr(4));
        auto line = x.second;

        // comma separated details
        std::vector<std::string> details;
        boost::split(details, line, boost::is_any_of(","));
        if (details.size() < 3) {
          g_log.warning() << "Invalid format for BILBY parameter " << x.first << std::endl;
          continue;
        }
        auto hdfTag = boost::algorithm::trim_copy(details[0]);
        try {
          // extract the parameter and add it to the parameter dictionary,
          // check the scale factor for numeric and string
          auto updateOk = false;
          if (!hdfTag.empty()) {
            if (isNumeric(details[1])) {
              bool baseLoaded = Anxs::loadNXDataSet(entry, hdfTag, tmpDouble, 0);
              bool timeLoaded = false;
              if (!baseLoaded) {
                auto key = (details.size() < 4) ? "mean" : boost::algorithm::trim_copy(details[3]);
                auto imap = ScanLogMap.find(key);
                Anxs::ScanLog scanLogMode =
                    (imap != ScanLogMap.end()) ? imap->second : Anxs::ScanLog::Mean; // default value
                timeLoaded = Anxs::extractTimedDataSet(entry, hdfTag, startTime, endTime, scanLogMode, tmpTimestamp,
                                                       tmpDouble, tmpString);
              }
              if (baseLoaded || timeLoaded) {
                auto factor = std::stod(details[1]);
                logParams[logTag] = factor * tmpDouble;
                updateOk = true;
                if (timeLoaded) {
                  traceStatistics<double>(entry, hdfTag, startTime, endTime, g_log);
                }
              }
            } else if (Anxs::loadNXString(entry, hdfTag, tmpString)) {
              logStrings[logTag] = tmpString;
              updateOk = true;
            }
          }
          if (!updateOk) {
            // if the hdf is missing the tag then add the default if
            // it is provided
            auto defValue = boost::algorithm::trim_copy(details[2]);
            if (!defValue.empty()) {
              if (isNumeric(defValue))
                logParams[logTag] = std::stod(defValue);
              else
                logStrings[logTag] = defValue;
              if (!hdfTag.empty())
                g_log.warning() << "Cannot find hdf parameter " << hdfTag << ", using default.\n";
            }
          }
        } catch (const std::invalid_argument &) {
          g_log.warning() << "Invalid format for BILBY parameter " << x.first << std::endl;
        }
      }
    }
  } catch (std::exception &ex) {
    g_log.warning() << "Failed to load instrument with error: " << ex.what()
                    << ". The current facility may not be fully "
                       "supported.\n";
  }
}

// instrument creation
void LoadBBY2::createInstrument(const Nexus::NXEntry &entry, uint64_t startTime, uint64_t endTime,
                                InstrumentInfo &instrumentInfo, std::map<std::string, double> &logParams,
                                std::map<std::string, std::string> &logStrings,
                                std::map<std::string, std::string> &allParams) {

  instrumentInfo.sample_name = "UNKNOWN";
  instrumentInfo.sample_description = "UNKNOWN";
  instrumentInfo.start_time = "2000-01-01T00:00:00";

  instrumentInfo.bm_counts = 0;
  instrumentInfo.att_pos = 0;
  instrumentInfo.master1_chopper_id = -1;
  instrumentInfo.master2_chopper_id = -1;

  instrumentInfo.is_tof = true;
  instrumentInfo.wavelength = 0.0;

  instrumentInfo.period_master = 0.0;
  instrumentInfo.period_slave = (1.0 / 50.0) * 1.0e6;
  instrumentInfo.phase_slave = 0.0;

  double tmp_double = 0.0f;
  int64_t tmp_int64 = 0;
  uint64_t tmp_timestamp = 0;
  std::string tmp_str;

  if (Anxs::loadNXDataSet(entry, "monitor/bm1_counts/value", tmp_int64, LAST_INDEX))
    instrumentInfo.bm_counts = tmp_int64;
  if (Anxs::loadNXDataSet(entry, "instrument/att_pos/value", tmp_double, LAST_INDEX))
    instrumentInfo.att_pos = boost::math::iround(tmp_double); // [1.0, 2.0, ..., 5.0]

  if (Anxs::loadNXString(entry, "sample/name", tmp_str))
    instrumentInfo.sample_name = tmp_str;
  if (Anxs::loadNXString(entry, "sample/description", tmp_str))
    instrumentInfo.sample_description = tmp_str;

  uint64_t epochStart{0};
  auto timeTag = (useHMScanTime ? "hmscan/time" : "scan_dataset/time");
  if (Anxs::loadNXDataSet(entry, timeTag, epochStart, 0)) {
    Types::Core::DateAndTime startDateTime(Anxs::epochRelDateTimeBase(epochStart));
    instrumentInfo.start_time = startDateTime.toISO8601String();
  }

  if (Anxs::loadNXDataSet(entry, "instrument/master1_chopper_id", tmp_int64, 0))
    instrumentInfo.master1_chopper_id = tmp_int64;
  if (Anxs::loadNXDataSet(entry, "instrument/master2_chopper_id", tmp_int64, 0))
    instrumentInfo.master2_chopper_id = tmp_int64;

  if (Anxs::loadNXString(entry, "instrument/detector/frame_source", tmp_str))
    instrumentInfo.is_tof = tmp_str == "EXTERNAL";

  if (Anxs::extractTimedDataSet(entry, "instrument/nvs067/lambda", startTime, endTime, Anxs::ScanLog::Mean,
                                tmp_timestamp, tmp_double, tmp_str))
    instrumentInfo.wavelength = tmp_double;

  if (Anxs::extractTimedDataSet(entry, "instrument/master_chopper_freq", startTime, endTime, Anxs::ScanLog::Mean,
                                tmp_timestamp, tmp_double, tmp_str) &&
      (tmp_double > 0.0f))
    instrumentInfo.period_master = 1.0 / tmp_double * 1.0e6;

  if (Anxs::extractTimedDataSet(entry, "instrument/t0_chopper_freq", startTime, endTime, Anxs::ScanLog::Mean,
                                tmp_timestamp, tmp_double, tmp_str) &&
      (tmp_double > 0.0f))
    instrumentInfo.period_slave = 1.0 / tmp_double * 1.0e6;

  if (Anxs::extractTimedDataSet(entry, "instrument/t0_chopper_phase", startTime, endTime, Anxs::ScanLog::Mean,
                                tmp_timestamp, tmp_double, tmp_str))
    instrumentInfo.phase_slave = tmp_double < 999.0 ? tmp_double : 0.0;

  // addnl trace message if needed
  traceStatistics<double>(entry, "instrument/nvs067/lambda", startTime, endTime, g_log);
  traceStatistics<double>(entry, "instrument/master_chopper_freq", startTime, endTime, g_log);
  traceStatistics<double>(entry, "instrument/t0_chopper_freq", startTime, endTime, g_log);
  traceStatistics<double>(entry, "instrument/t0_chopper_phase", startTime, endTime, g_log);

  loadInstrumentParameters(entry, startTime, endTime, logParams, logStrings, allParams);

  // Ltof_det_value is not present for monochromatic data so check
  // and replace with default
  auto findLtof = logParams.find("Ltof_det_value");
  if (findLtof != logParams.end()) {
    logParams["L1_chopper_value"] = logParams["Ltof_det_value"] - logParams["L2_det_value"];
  } else {
    logParams["L1_chopper_value"] = 18.4726;
    g_log.warning() << "Cannot recover parameter 'L1_chopper_value'"
                    << ", using default.\n";
  }
}

} // namespace Mantid::DataHandling
