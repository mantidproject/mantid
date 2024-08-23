// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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

// register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadBBY)

// consts
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

using ANSTO::EventVector_pt;

template <typename TYPE>
void AddSinglePointTimeSeriesProperty(API::LogManager &logManager, const std::string &time, const std::string &name,
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
    if ((len > 4) && (subFile.find_first_of("\\/", 0, 2) == std::string::npos)) {
      if ((subFile.rfind(".hdf") == len - 4) && (subFile.compare(0, 3, "BBY") == 0))
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
  createInstrument(tarFile, instrumentInfo, logParams, logStrings, allParams);

  // set the units
  if (instrumentInfo.is_tof)
    eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  else
    eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Wavelength");

  eventWS->setYUnit("Counts");

  // set title
  const std::vector<std::string> &subFiles = tarFile.files();
  const auto it = std::find_if(subFiles.cbegin(), subFiles.cend(),
                               [](const auto &subFile) { return subFile.compare(0, 3, "BBY") == 0; });
  if (it != subFiles.cend()) {
    std::string title = *it;

    if (title.rfind(".hdf") == title.length() - 4)
      title.resize(title.length() - 4);

    if (title.rfind(".nx") == title.length() - 3)
      title.resize(title.length() - 3);

    eventWS->setTitle(title);
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

  // get the start time from the file
  Types::Core::DateAndTime startTime(instrumentInfo.start_time);
  auto startInNanosec = startTime.totalNanoseconds();

  // count total events per pixel to reserve necessary memory
  ANSTO::EventCounter eventCounter(roi, HISTO_BINS_Y, period, shift, startInNanosec, tofMinBoundary, tofMaxBoundary,
                                   timeMinBoundary, timeMaxBoundary, eventCounts);

  loadEvents(prog, "loading neutron counts", tarFile, eventCounter);

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

    loadEvents(prog, "loading neutron events (TOF)", tarFile, eventAssigner);
  } else {
    ANSTO::EventAssignerFixedWavelength eventAssigner(roi, HISTO_BINS_Y, instrumentInfo.wavelength, period, shift,
                                                      startInNanosec, tofMinBoundary, tofMaxBoundary, timeMinBoundary,
                                                      timeMaxBoundary, eventVectors);

    loadEvents(prog, "loading neutron events (Wavelength)", tarFile, eventAssigner);
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
    maskingAlg->setProperty("Workspace", eventWS);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }

  // set log values
  API::LogManager &logManager = eventWS->mutableRun();

  auto frame_count = static_cast<int>(eventCounter.numFrames());

  logManager.addProperty("filename", filename);
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
  AddSinglePointTimeSeriesProperty(logManager, time_str, "wavelength", instrumentInfo.wavelength);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "master1_chopper_id", instrumentInfo.master1_chopper_id);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "master2_chopper_id", instrumentInfo.master2_chopper_id);

  for (auto &x : logStrings) {
    logManager.addProperty(x.first, x.second);
  }
  for (auto &x : logParams) {
    AddSinglePointTimeSeriesProperty(logManager, time_str, x.first, x.second);
  }

  auto loadInstrumentAlg = createChildAlgorithm("LoadInstrument");
  loadInstrumentAlg->setProperty("Workspace", eventWS);
  loadInstrumentAlg->setPropertyValue("InstrumentName", "BILBY");
  loadInstrumentAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
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
void LoadBBY::loadInstrumentParameters(const NeXus::NXEntry &entry, std::map<std::string, double> &logParams,
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
    float tmpFloat = 0.0f;
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
              if (loadNXDataSet(entry, hdfTag, tmpFloat)) {
                auto factor = std::stod(details[1]);
                logParams[logTag] = factor * tmpFloat;
                updateOk = true;
              }
            } else if (loadNXString(entry, hdfTag, tmpString)) {
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
void LoadBBY::createInstrument(ANSTO::Tar::File &tarFile, InstrumentInfo &instrumentInfo,
                               std::map<std::string, double> &logParams, std::map<std::string, std::string> &logStrings,
                               std::map<std::string, std::string> &allParams) {

  const double toMeters = 1.0 / 1000;

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

  // extract log and hdf file
  const std::vector<std::string> &files = tarFile.files();
  auto file_it = std::find_if(files.cbegin(), files.cend(),
                              [](const std::string &file) { return file.rfind(".hdf") == file.length() - 4; });
  if (file_it != files.end()) {
    tarFile.select(file_it->c_str());
    // extract hdf file into tmp file
    Poco::TemporaryFile hdfFile;
    std::shared_ptr<FILE> handle(fopen(hdfFile.path().c_str(), "wb"), fclose);
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
        instrumentInfo.att_pos = boost::math::iround(tmp_float); // [1.0, 2.0, ..., 5.0]

      if (loadNXString(entry, "sample/name", tmp_str))
        instrumentInfo.sample_name = tmp_str;
      if (loadNXString(entry, "sample/description", tmp_str))
        instrumentInfo.sample_description = tmp_str;
      if (loadNXString(entry, "start_time", tmp_str))
        instrumentInfo.start_time = tmp_str;

      if (loadNXDataSet(entry, "instrument/master1_chopper_id", tmp_int32))
        instrumentInfo.master1_chopper_id = tmp_int32;
      if (loadNXDataSet(entry, "instrument/master2_chopper_id", tmp_int32))
        instrumentInfo.master2_chopper_id = tmp_int32;

      if (loadNXString(entry, "instrument/detector/frame_source", tmp_str))
        instrumentInfo.is_tof = tmp_str == "EXTERNAL";
      if (loadNXDataSet(entry, "instrument/nvs067/lambda", tmp_float))
        instrumentInfo.wavelength = tmp_float;

      if (loadNXDataSet(entry, "instrument/master_chopper_freq", tmp_float) && (tmp_float > 0.0f))
        instrumentInfo.period_master = 1.0 / tmp_float * 1.0e6;
      if (loadNXDataSet(entry, "instrument/t0_chopper_freq", tmp_float) && (tmp_float > 0.0f))
        instrumentInfo.period_slave = 1.0 / tmp_float * 1.0e6;
      if (loadNXDataSet(entry, "instrument/t0_chopper_phase", tmp_float))
        instrumentInfo.phase_slave = tmp_float < 999.0 ? tmp_float : 0.0;

      loadInstrumentParameters(entry, logParams, logStrings, allParams);

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
  }

  // patching
  file_it = std::find(files.cbegin(), files.cend(), "History.log");
  if (file_it != files.cend()) {
    tarFile.select(file_it->c_str());
    std::string logContent;
    logContent.resize(tarFile.selected_size());
    tarFile.read(&logContent[0], logContent.size());
    std::istringstream data(logContent);
    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> conf(new Poco::Util::PropertyFileConfiguration(data));

    if (conf->hasProperty("Bm1Counts"))
      instrumentInfo.bm_counts = conf->getInt("Bm1Counts");
    if (conf->hasProperty("AttPos"))
      instrumentInfo.att_pos = boost::math::iround(conf->getDouble("AttPos"));

    if (conf->hasProperty("SampleName"))
      instrumentInfo.sample_name = conf->getString("SampleName");

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

    if (conf->hasProperty("SampleAperture"))
      logParams["sample_aperture"] = conf->getDouble("SampleAperture");
    if (conf->hasProperty("SourceAperture"))
      logParams["source_aperture"] = conf->getDouble("SourceAperture");
    if (conf->hasProperty("L1"))
      logParams["L1_source_value"] = conf->getDouble("L1") * toMeters;
    if (conf->hasProperty("LTofDet"))
      logParams["L1_chopper_value"] = conf->getDouble("LTofDet") * toMeters - logParams["L2_det_value"];
    if (conf->hasProperty("L2Det"))
      logParams["L2_det_value"] = conf->getDouble("L2Det") * toMeters;

    if (conf->hasProperty("L2CurtainL"))
      logParams["L2_curtainl_value"] = conf->getDouble("L2CurtainL") * toMeters;
    if (conf->hasProperty("L2CurtainR"))
      logParams["L2_curtainr_value"] = conf->getDouble("L2CurtainR") * toMeters;
    if (conf->hasProperty("L2CurtainU"))
      logParams["L2_curtainu_value"] = conf->getDouble("L2CurtainU") * toMeters;
    if (conf->hasProperty("L2CurtainD"))
      logParams["L2_curtaind_value"] = conf->getDouble("L2CurtainD") * toMeters;

    if (conf->hasProperty("CurtainL"))
      logParams["D_curtainl_value"] = conf->getDouble("CurtainL") * toMeters;
    if (conf->hasProperty("CurtainR"))
      logParams["D_curtainr_value"] = conf->getDouble("CurtainR") * toMeters;
    if (conf->hasProperty("CurtainU"))
      logParams["D_curtainu_value"] = conf->getDouble("CurtainU") * toMeters;
    if (conf->hasProperty("CurtainD"))
      logParams["D_curtaind_value"] = conf->getDouble("CurtainD") * toMeters;
  }
}

// load nx dataset
template <class T> bool LoadBBY::loadNXDataSet(const NeXus::NXEntry &entry, const std::string &path, T &value) {
  try {
    NeXus::NXDataSetTyped<T> dataSet = entry.openNXDataSet<T>(path);
    dataSet.load();

    value = *dataSet();
    return true;
  } catch (std::runtime_error &) {
    return false;
  }
}
bool LoadBBY::loadNXString(const NeXus::NXEntry &entry, const std::string &path, std::string &value) {
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
void LoadBBY::loadEvents(API::Progress &prog, const char *progMsg, ANSTO::Tar::File &tarFile,
                         EventProcessor &eventProcessor) {
  prog.doReport(progMsg);

  // select bin file
  int64_t fileSize = 0;
  const std::vector<std::string> &files = tarFile.files();
  const auto found = std::find_if(files.cbegin(), files.cend(),
                                  [](const auto &file) { return file.rfind(".bin") == file.length() - 4; });
  if (found != files.cend()) {
    tarFile.select(found->c_str());
    fileSize = tarFile.selected_size();
  }

  // for progress notifications
  ANSTO::ProgressTracker progTracker(prog, progMsg, fileSize, Progress_LoadBinFile);

  uint32_t x = 0; // 9 bits [0-239] tube number
  uint32_t y = 0; // 8 bits [0-255] position along tube

  // uint v = 0; // 0 bits [     ]
  // uint w = 0; // 0 bits [     ] energy
  uint32_t dt = 0;
  double tof = 0.0;

  if ((fileSize == 0) || !tarFile.skip(128))
    return;

  int state = 0;
  int invalidEvents = 0;
  uint32_t c;
  while ((c = static_cast<uint32_t>(tarFile.read_byte())) != static_cast<uint32_t>(-1)) {

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
        // cannot ignore the dt contrbition even if the event
        // is out of bounds as it is used in the encoding process
        tof += static_cast<int>(dt) * 0.1;
        invalidEvents++;
      } else {
        // conversion from 100 nanoseconds to 1 microsecond
        tof += static_cast<int>(dt) * 0.1;

        eventProcessor.addEvent(x, y, tof);
      }

      progTracker.update(tarFile.selected_position());
    }
  }
  if (invalidEvents > 0) {
    g_log.warning() << "BILBY loader dropped " << invalidEvents << " invalid event(s)" << std::endl;
  }
}

} // namespace Mantid::DataHandling
