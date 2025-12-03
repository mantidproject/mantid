// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNGEM.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <ranges>

namespace Mantid::DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadNGEM)

// Constants and helper functions.
namespace {
constexpr int NUM_OF_SPECTRA = 16384;

/**
 * @brief Byte swap a 64 bit word as the nGEM detector outputs in big endian
 * format. So must be swapped to have correct values on x86 and x64
 * architectures.
 *
 * @param word The 64 bit word to swap.
 * @return uint64_t The swapped word.
 */
uint64_t swapUint64(uint64_t word) {
  word = ((word << 8) & 0xFF00FF00FF00FF00ULL) | ((word >> 8) & 0x00FF00FF00FF00FFULL);
  word = ((word << 16) & 0xFFFF0000FFFF0000ULL) | ((word >> 16) & 0x0000FFFF0000FFFFULL);
  return (word << 32) | (word >> 32);
}

/**
 * @brief Correct a big endian event to be compatible with x64 and x86
 * architecture.
 *
 * @param bigEndian The big endian formatted event.
 * @param smallEndian The resulting small endian formatted event.
 */
void correctForBigEndian(const EventUnion &bigEndian, EventUnion &smallEndian) {
  smallEndian.splitWord.words[0] = swapUint64(bigEndian.splitWord.words[1]);
  smallEndian.splitWord.words[1] = swapUint64(bigEndian.splitWord.words[0]);
}

/**
 * @brief Add a frame to the main set of events.
 *
 * @param rawFrames The number of T0 Events detected so far.
 * @param goodFrames The number of good frames detected so far.
 * @param eventCountInFrame The number of events in the current frame.
 * @param minEventsReq The number of events required to be a good frame.
 * @param maxEventsReq The max events allowed to be a good frame.
 * @param frameEventCounts A vector of the number of events in each good frame.
 * @param events The main set of events for the data so far.
 * @param eventsInFrame The set of events for the current frame.
 */
void addFrameToOutputWorkspace(int &rawFrames, int &goodFrames, const int eventCountInFrame, const int minEventsReq,
                               const int maxEventsReq, MantidVec &frameEventCounts,
                               std::vector<DataObjects::EventList> &events,
                               std::vector<DataObjects::EventList> &eventsInFrame) {
  ++rawFrames;
  if (eventCountInFrame >= minEventsReq && eventCountInFrame <= maxEventsReq) {
    // Add number of event counts to workspace.
    frameEventCounts.emplace_back(eventCountInFrame);
    ++goodFrames;

    PARALLEL_FOR_NO_WSP_CHECK()
    // Add events that match parameters to workspace
    for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
      if (eventsInFrame[i].getNumberEvents() > 0) {
        events[i] += eventsInFrame[i];
        eventsInFrame[i].clear();
      }
    }
  } else {
    // clear event list in frame in preparation for next frame
    PARALLEL_FOR_NO_WSP_CHECK()
    // Add events that match parameters to workspace
    for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
      eventsInFrame[i].clear();
    }
  }
}

/**
 * @brief Add a frame to the main set of events.
 *
 * @param rawFrames The number of T0 Events detected so far.
 * @param goodFrames The number of good frames detected so far.
 * @param eventCountInFrame The number of events in the current frame.
 * @param minEventsReq The number of events required to be a good frame.
 * @param maxEventsReq The max events allowed to be a good frame.
 * @param frameEventCounts A vector of the number of events in each good frame.
 * @param counts A vector of counts, by pixel, for the data so far.
 * @param countsInFrame A vector of counts, by pixel, for the current frame.
 */
void addFrameToOutputWorkspace(int &rawFrames, int &goodFrames, const int eventCountInFrame, const int minEventsReq,
                               const int maxEventsReq, MantidVec &frameEventCounts,
                               std::vector<std::vector<double>> &counts,
                               std::vector<std::vector<double>> &countsInFrame) {
  ++rawFrames;
  if (eventCountInFrame >= minEventsReq && eventCountInFrame <= maxEventsReq) {
    // Add number of event counts to workspace.
    frameEventCounts.emplace_back(eventCountInFrame);
    ++goodFrames;

    PARALLEL_FOR_NO_WSP_CHECK()
    // Add events that match parameters to workspace
    for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
      auto &countsInFramePixel = countsInFrame[i];
      for (size_t j = 0; j < countsInFramePixel.size(); ++j) {
        auto &countsInFramePixelByBin = countsInFramePixel[j];
        if (countsInFramePixelByBin > 0) {
          counts[i][j] += countsInFramePixelByBin;
          countsInFramePixelByBin = 0;
        }
      }
    }
  } else {
    // clear event list in frame in preparation for next frame
    PARALLEL_FOR_NO_WSP_CHECK()
    // Add events that match parameters to workspace
    for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
      auto &countsInFramePixel = countsInFrame[i];
      std::fill(countsInFramePixel.begin(), countsInFramePixel.end(), 0);
    }
  }
}

/**
 * @brief Creates an event workspace and fills it with the data.
 *
 * @param maxToF The largest ToF seen so far.
 * @param binWidth The width of each bin.
 * @param events The main events data.
 */
API::MatrixWorkspace_sptr createEventWorkspace(const double maxToF, const double binWidth,
                                               const std::vector<DataObjects::EventList> &events) {
  // Round up number of bins needed
  std::vector<double> xAxis(int(std::ceil(maxToF / binWidth)));
  std::generate(xAxis.begin(), xAxis.end(), [i = 0, &binWidth]() mutable { return binWidth * i++; });

  DataObjects::EventWorkspace_sptr dataWorkspace = DataObjects::create<DataObjects::EventWorkspace>(
      NUM_OF_SPECTRA, HistogramData::Histogram(HistogramData::BinEdges(xAxis)));
  PARALLEL_FOR_NO_WSP_CHECK()
  for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
    dataWorkspace->getSpectrum(i) = events[i];
    dataWorkspace->getSpectrum(i).setSpectrumNo(i + 1);
    dataWorkspace->getSpectrum(i).setDetectorID(i + 1);
  }
  dataWorkspace->setAllX(HistogramData::BinEdges{xAxis});
  dataWorkspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  dataWorkspace->setYUnit("Counts");
  return std::dynamic_pointer_cast<API::MatrixWorkspace>(dataWorkspace);
}

/**
 * @brief Creates an event workspace and fills it with the data.
 *
 * @param binEdges A vector of bin edges to be shared between histograms.
 * @param counts A vector containing a sub vector for each spectra, each containing counts.
 */
API::MatrixWorkspace_sptr createHistogramWorkspace(std::vector<double> &&binEdges,
                                                   std::vector<std::vector<double>> &&counts) {
  const HistogramData::BinEdges bins{std::move(binEdges)};
  API::MatrixWorkspace_sptr dataWorkspace = DataObjects::create<DataObjects::Workspace2D>(NUM_OF_SPECTRA, bins);
  PARALLEL_FOR_NO_WSP_CHECK()
  for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
    dataWorkspace->setHistogram(i, bins, HistogramData::Counts{std::move(counts[i])});
    dataWorkspace->getSpectrum(i).setSpectrumNo(i + 1);
    dataWorkspace->getSpectrum(i).setDetectorID(i + 1);
  }
  dataWorkspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  dataWorkspace->setYUnit("Counts");
  return dataWorkspace;
}

/**
 * @brief Add a value to the sample logs.
 *
 * @param name The name of the log.
 * @param value The content of the log.
 * @param ws The workspace to add the log to.
 */
template <typename ValueType>
void addToSampleLog(const std::string &name, const ValueType &value, API::MatrixWorkspace_sptr &ws) {
  ws->mutableRun().addProperty(name, value, false);
}

/**
 * @brief Insert validation results into map, if there is an error.
 * @param result error property and associated error message to insert into map
 * @param results map containing validation results
 */
void insertValidationResult(const std::pair<std::string, std::string> &result,
                            std::map<std::string, std::string> &results) {
  if (result.first.empty()) {
    return;
  }
  results.insert(result);
}

/**
 * @brief Calculate bin edges from min/max ToF and bin width.
 * @param minToF minimum of first histo bin
 * @param maxToF maximum of last histo bin
 * @param binWidth width of each histo bin
 * @return std::vector<double> containing bin edge values.
 */
std::vector<double> calculateBinEdges(const double minToF, const double maxToF, const double binWidth) {
  const int numBins = static_cast<int>(std::ceil((maxToF - minToF) / binWidth));
  auto custom_range =
      std::views::iota(0) | std::views::transform([&binWidth, &minToF](int n) { return minToF + (n * binWidth); });
  return {custom_range.begin(), custom_range.begin() + numBins + 1};
}
} // namespace

/**
 * @brief The confidence that a file can be loaded.
 *
 * @param descriptor A description of the file.
 * @return int The level of certainty that the file can be loaded with this
 * loader.
 */
int LoadNGEM::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension() == ".edb") {
    return 95;
  } else {
    return 0;
  }
}

/**
 * @brief Initialisation of the algorithm, setting up the properties.
 */
void LoadNGEM::init() {
  // Filename property.
  const std::vector<std::string> extentions{".edb"};
  declareProperty(std::make_unique<API::MultipleFileProperty>("Filename", extentions),
                  "The name of the nGEM file to load. Selecting multiple files will "
                  "combine them into one workspace.");
  // Output workspace
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output workspace");

  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);

  auto mustBePositiveDbl = std::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositiveDbl->setLower(0.0);

  // Bin Width
  declareProperty("BinWidth", 10.0, mustBePositiveDbl, "The width of the time bins in the output.");
  declareProperty(
      "MinToF", std::numeric_limits<double>::max(),
      "The minimum ToF bin edge, inclusive. Required if not PreserveEvents. If PreserveEvents and default, value will "
      "be dervied from event data.");
  declareProperty(
      "MaxToF", -std::numeric_limits<double>::max(),
      "The maximum ToF bin edge, exclusive. Required if not PreserveEvents. If PreserveEvents and default, value will "
      "be dervied from event data.");

  declareProperty("MinEventsPerFrame", 0, mustBePositive,
                  "The minimum number of events required in a frame before a "
                  "it is considered 'good'.");
  declareProperty("MaxEventsPerFrame", EMPTY_INT(), mustBePositive,
                  "The maximum number of events allowed in a frame to be "
                  "considered 'good'.");
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<bool>>("GenerateEventsPerFrame", false, Kernel::Direction::Input),
      "Generate a workspace to show the number of events captured by each "
      "frame. (optional, default False).");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("PreserveEvents", true, Kernel::Direction::Input),
                  "Algorithm preserves events, generating an event workspace. If False, events are not preserved and a "
                  "Workspace 2D is generated (reduced memory usage). "
                  "(optional, default True).");
}

/**
 * @brief Execute the algorithm.
 */
void LoadNGEM::exec() {
  progress(0);

  std::vector<std::vector<std::string>> filePaths = getProperty("Filename");
  const int minEventsReq(getProperty("MinEventsPerFrame"));
  const int maxEventsReq(getProperty("MaxEventsPerFrame"));
  double minToF(getProperty("MinToF"));
  double maxToF(getProperty("MaxToF"));
  const double binWidth(getProperty("BinWidth"));
  const bool preserveEvents(getProperty("PreserveEvents"));

  LoadDataResult res;
  if (preserveEvents) {
    res = readDataAsEvents(minToF, maxToF, binWidth, minEventsReq, maxEventsReq, filePaths);
  } else {
    res = readDataAsHistograms(minToF, maxToF, binWidth, minEventsReq, maxEventsReq, filePaths);
  }

  addToSampleLog("raw_frames", res.rawFrames, res.dataWorkspace);
  addToSampleLog("good_frames", res.goodFrames, res.dataWorkspace);
  addToSampleLog("max_ToF", maxToF, res.dataWorkspace);
  addToSampleLog("min_ToF", minToF, res.dataWorkspace);

  loadInstrument(res.dataWorkspace);

  setProperty("OutputWorkspace", res.dataWorkspace);
  if (this->getProperty("GenerateEventsPerFrame")) {
    createCountWorkspace(res.frameEventCounts);
  }
  m_fileCount = 0;
  progress(1.00);
}

/**
 * @brief Load a single file into histograms or events.
 *
 * @param filePath The path to the file.
 * @param eventCountInFrame The number of events in the current frame.
 * @param minToF The lowest detected ToF
 * @param maxToF The highest detected ToF
 * @param binWidth The width of histogram bins in ToF
 * @param rawFrames The number of T0 Events detected so far.
 * @param goodFrames The number of good frames detected so far.
 * @param minEventsReq The number of events required to be a good frame.
 * @param maxEventsReq The max events allowed to be a good frame.
 * @param frameEventCounts A vectot tracking event counts per frame.
 * @param totalFilePaths The total number of file paths.
 * @param strategy Object encapsulating histo/event specific logic
 */
void LoadNGEM::loadSingleFile(const std::vector<std::string> &filePath, int &eventCountInFrame, double &minToF,
                              double &maxToF, const double binWidth, int &rawFrames, int &goodFrames,
                              const int minEventsReq, const int maxEventsReq, MantidVec &frameEventCounts,
                              const size_t totalFilePaths, std::shared_ptr<LoadDataStrategyBase> strategy) {
  // Create file reader
  if (filePath.size() > 1) {
    throw std::runtime_error("Invalid filename parameter.");
  }
  std::ifstream file(filePath[0].c_str(), std::ifstream::binary);
  if (!file.is_open()) {
    throw std::runtime_error("File could not be found.");
  }

  const size_t totalNumEvents = verifyFileSize(file) / 16;
  constexpr size_t SKIP_WORD_SIZE = 4;
  size_t numProcessedEvents = 0;
  size_t numWordsSkipped = 0;

  while (true) {
    // Load an event into the variable.
    // Occasionally we may get a file where the first event has been chopped,
    // so we seek to the start of a valid event.
    // Chopping only seems to occur on a 4 byte word, hence seekg() of 4
    EventUnion event, eventBigEndian;
    bool isEventInvalid = true;
    bool isNotEOFAfterSkip = true;
    do {
      file.read(reinterpret_cast<char *>(&eventBigEndian), sizeof(eventBigEndian));
      // Correct for the big endian format of nGEM datafile.
      correctForBigEndian(eventBigEndian, event);
      isEventInvalid = !event.generic.check();
      if (isEventInvalid) {
        isNotEOFAfterSkip = !file.seekg(SKIP_WORD_SIZE, std::ios_base::cur).eof();
        if (isNotEOFAfterSkip) {
          ++numWordsSkipped;
        }
      }
    } while (isEventInvalid && isNotEOFAfterSkip);
    if (file.eof()) {
      break; // we have either not read an event, or only read part of one
    }
    if (event.coincidence.check()) { // Check for coincidence event.
      ++eventCountInFrame;
      size_t pixel = event.coincidence.getPixel();
      // Convert to microseconds (us)
      const double tof = event.coincidence.timeOfFlight / 1000.0;
      strategy->addEvent(minToF, maxToF, tof, binWidth,
                         pixel);      // If no min and max provided by user, values calculated and mutated
    } else if (event.tZero.check()) { // Check for T0 event.
      strategy->addFrame(rawFrames, goodFrames, eventCountInFrame, minEventsReq, maxEventsReq, frameEventCounts);
      if (reportProgressAndCheckCancel(numProcessedEvents, eventCountInFrame, totalNumEvents, totalFilePaths)) {
        return;
      }
    } else if (event.generic.check()) { // match all other events and notify.
      g_log.warning() << "Unexpected event type ID=" << event.generic.id << " loaded.\n";
    } else { // if we were to get to here, must be a corrupt event
      g_log.warning() << "Corrupt event detected.\n";
    }
  }
  if (numWordsSkipped > 0) {
    g_log.warning() << SKIP_WORD_SIZE * numWordsSkipped
                    << " bytes of file data were skipped when locating valid events.\n";
  }
  g_log.information() << "Finished loading a file.\n";
  ++m_fileCount;
}
/**
 * @brief Ensure that the file fits into 16, as the detector spits out 128 bit
 * words (16 bytes)
 *
 * @param file The file to check.
 * @return size_t The size of the file.
 */
size_t LoadNGEM::verifyFileSize(std::ifstream &file) {
  // Check that the file fits into 16 byte sections.
  file.seekg(0, file.end);
  size_t size = file.tellg();
  if (size % 16 != 0) {
    g_log.warning() << "Invalid file size. File is size is " << size
                    << " bytes which is not a multiple of 16. There may be some bytes "
                       "missing from the data. \n";
  }
  file.seekg(0);
  return size;
}

/**
 * @brief Report the progress of the loader through the current file.
 *
 * @param numProcessedEvents The number of events processed so far.
 * @param eventCountInFrame The number of events in the current frame.
 * @param totalNumEvents The total number of events in the file.
 * @param totalFilePaths The total number of file paths to process.
 * @return true If user has cancelled the load.
 * @return false If the user has not cancelled.
 */
bool LoadNGEM::reportProgressAndCheckCancel(size_t &numProcessedEvents, int &eventCountInFrame,
                                            const size_t totalNumEvents, const size_t totalFilePaths) {
  numProcessedEvents += eventCountInFrame;
  std::string message(std::to_string(m_fileCount) + "/" + std::to_string(totalFilePaths));
  progress(double(numProcessedEvents) / double(totalNumEvents) / 1.11111, message);
  eventCountInFrame = 0;
  // Check for cancel flag.
  return this->getCancel();
}

/**
 * @brief Create a count workspace to allow for the selection of "good"
 * frames.
 *
 * @param frameEventCounts A Vector of the number of events per frame.
 */
void LoadNGEM::createCountWorkspace(const std::vector<double> &frameEventCounts) {
  std::vector<double> xAxisCounts(frameEventCounts.size() + 1);
  std::generate(xAxisCounts.begin(), xAxisCounts.end(), [n = 0.0]() mutable { return ++n; });

  DataObjects::Workspace2D_sptr countsWorkspace =
      DataObjects::create<DataObjects::Workspace2D>(1, HistogramData::Histogram(HistogramData::BinEdges(xAxisCounts)));

  countsWorkspace->mutableY(0) = frameEventCounts;
  std::string countsWorkspaceName(this->getProperty("OutputWorkspace"));
  countsWorkspaceName.append("_event_counts");
  countsWorkspace->setYUnit("Counts");
  std::shared_ptr<Kernel::Units::Label> XLabel =
      std::dynamic_pointer_cast<Kernel::Units::Label>(Kernel::UnitFactory::Instance().create("Label"));
  XLabel->setLabel("Frame");
  countsWorkspace->getAxis(0)->unit() = XLabel;

  this->declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>("CountsWorkspace", countsWorkspaceName,
                                                                                 Kernel::Direction::Output),
                        "Counts of events per frame.");
  progress(1.00);
  this->setProperty("CountsWorkspace", countsWorkspace);
}

/**
 * @brief Load the nGEM instrument into a workspace.
 *
 * @param dataWorkspace The workspace to load into.
 */
void LoadNGEM::loadInstrument(API::MatrixWorkspace_sptr &dataWorkspace) {
  auto loadInstrument = this->createChildAlgorithm("LoadInstrument");
  loadInstrument->setPropertyValue("InstrumentName", "NGEM");
  loadInstrument->setProperty<API::MatrixWorkspace_sptr>("Workspace", dataWorkspace);
  loadInstrument->setProperty("RewriteSpectraMap", Kernel::OptionalBool(false));
  loadInstrument->execute();
}

/**
 * @brief Validate inputs into the loader GUI.
 *
 * @return std::map<std::string, std::string> A map that is empty if there are
 * no issues, and contains a key of the input field and a value of the error
 * message otherwise.
 */
std::map<std::string, std::string> LoadNGEM::validateInputs() {
  std::map<std::string, std::string> results;
  insertValidationResult(validateEventsPerFrame(), results);
  insertValidationResult(validateMinMaxToF(), results);
  return results;
}

/**
 * @brief validate min/max events per frame inputs.
 *
 * @return std::pair<std::string, std::string> subject property: error message.
 */
std::pair<std::string, std::string> LoadNGEM::validateEventsPerFrame() {
  const int MinEventsPerFrame = getProperty("MinEventsPerFrame");
  const int MaxEventsPerFrame = getProperty("MaxEventsPerFrame");
  if (MaxEventsPerFrame < MinEventsPerFrame) {
    return {"MaxEventsPerFrame", "MaxEventsPerFrame is less than MinEvents per frame"};
  }
  return {};
}

/**
 * @brief validate min/max ToF inputs.
 *
 * @return std::pair<std::string, std::string> subject property: error message.
 */
std::pair<std::string, std::string> LoadNGEM::validateMinMaxToF() {
  const double minToF = getProperty("MinToF");
  const double maxToF = getProperty("MaxToF");
  const bool preserveEvents = getProperty("PreserveEvents");
  if (preserveEvents) {
    if (isDefault("MinToF") && isDefault("MaxToF")) {
      return {};
    } else if (isDefault("MinToF")) {
      return {"MinToF", "Please supply both, or neither, Min and MaxToF if PreserveEvents is True"};
    } else if (isDefault("MaxToF")) {
      return {"MaxToF", "Please supply both, or neither, Min and MaxToF if PreserveEvents is True"};
    }
  } else {
    if (isDefault("MinToF")) {
      return {"MinToF", "MinToF must be supplied if PreserveEvents is False"};
    } else if (isDefault("MaxToF")) {
      return {"MaxToF", "MaxToF must be supplied if PreserveEvents is False"};
    }
  }
  if (maxToF <= minToF) {
    return {"MaxToF", "MaxToF is less than or equal to MinToF"};
  }
  return {};
}

/**
 * @brief Read data from provided file paths into event lists, output an event workspace.
 *
 * @param minToF The minimum ToF value to consider
 * @param maxToF The maximum ToF value to consider
 * @param binWidth The width of bins that the events will be projected into by the resultant workspace
 * @param minEventsReq The minimum events required in to consider a frame good.
 * @param maxEventsReq The maximum events required in to consider a frame good.
 * @param filePaths Paths to the data file to read.
 * @return A struct containing the output workspace and variables tracking frames
 */
LoadDataResult LoadNGEM::readDataAsEvents(double &minToF, double &maxToF, const double binWidth, const int minEventsReq,
                                          const int maxEventsReq,
                                          const std::vector<std::vector<std::string>> &filePaths) {
  int eventCountInFrame{0};
  int rawFrames{0};
  int goodFrames{0};
  std::vector<double> frameEventCounts;
  API::MatrixWorkspace_sptr dataWorkspace;
  size_t totalFilePaths(filePaths.size());

  progress(0.04);
  auto strategy = std::make_shared<LoadDataStrategyEvent>();
  for (const auto &filePath : filePaths) {
    loadSingleFile(filePath, eventCountInFrame, minToF, maxToF, binWidth, rawFrames, goodFrames, minEventsReq,
                   maxEventsReq, frameEventCounts, totalFilePaths, strategy);
  }
  // Add the final frame of events (as they are not followed by a T0 event)
  strategy->addFrame(rawFrames, goodFrames, eventCountInFrame, minEventsReq, maxEventsReq, frameEventCounts);
  progress(0.90);
  dataWorkspace = createEventWorkspace(maxToF, binWidth, strategy->getEvents());

  return {rawFrames, goodFrames, frameEventCounts, dataWorkspace};
}

/**
 * @brief Read data from provided file paths into a histograms, output a workspace2D.
 *
 * @param minToF The minimum ToF value to consider
 * @param maxToF The maximum ToF value to consider
 * @param binWidth The width of bins that the events will be projected into by the resultant workspace
 * @param minEventsReq The minimum events required in to consider a frame good.
 * @param maxEventsReq The maximum events required in to consider a frame good.
 * @param filePaths Paths to the data file to read.
 * @return A struct containing the output workspace and variables tracking frames
 */
LoadDataResult LoadNGEM::readDataAsHistograms(double &minToF, double &maxToF, const double binWidth,
                                              const int minEventsReq, const int maxEventsReq,
                                              const std::vector<std::vector<std::string>> &filePaths) {
  int rawFrames{0};
  int goodFrames{0};
  int eventCountInFrame{0};
  std::vector<double> frameEventCounts;
  API::MatrixWorkspace_sptr dataWorkspace;
  size_t totalFilePaths(filePaths.size());

  progress(0.04);
  auto strategy = std::make_shared<LoadDataStrategyHisto>(minToF, maxToF, binWidth);
  for (const auto &filePath : filePaths) {
    loadSingleFile(filePath, eventCountInFrame, minToF, maxToF, binWidth, rawFrames, goodFrames, minEventsReq,
                   maxEventsReq, frameEventCounts, totalFilePaths, strategy);
  }
  // Add the final frame of events (as they are not followed by a T0 event)
  strategy->addFrame(rawFrames, goodFrames, eventCountInFrame, minEventsReq, maxEventsReq, frameEventCounts);
  progress(0.90);
  dataWorkspace = createHistogramWorkspace(std::move(strategy->getBinEdges()), std::move(strategy->getCounts()));
  return {rawFrames, goodFrames, frameEventCounts, dataWorkspace};
}

/**
 * @brief Add an event to the relevant histogram.
 *
 * @param minToF The minimum ToF value to consider
 * @param maxToF The maximum ToF value to consider
 * @param tof The time of flight associated witht he evenr
 * @param binWidth The width of each histogram bin
 * @param pixel The detector pixel index.
 */
void LoadDataStrategyHisto::addEvent(double &minToF, double &maxToF, const double tof, const double binWidth,
                                     const size_t pixel) {
  if (tof >= maxToF || tof < minToF) {
    return;
  }
  const int bin_idx = static_cast<int>(std::floor((tof - minToF) / binWidth));
  m_countsInFrame[pixel][bin_idx]++;
}

/**
 * @brief Add a completed frame to the count-by-spectra vector.
 *
 * @param rawFrames The number of raw frames read from file
 * @param goodFrames The number of frames that fulfill good quality criteria
 * @param eventCountInFrame The event counts in current frame
 * @param minEventsReq The minimum events required to be considered a good frame
 * @param maxEventsReq The maximum events required to be considered a good frame
 * @param frameEventCounts Event count by frame
 */
void LoadDataStrategyHisto::addFrame(int &rawFrames, int &goodFrames, const int eventCountInFrame,
                                     const int minEventsReq, const int maxEventsReq, MantidVec &frameEventCounts) {
  addFrameToOutputWorkspace(rawFrames, goodFrames, eventCountInFrame, minEventsReq, maxEventsReq, frameEventCounts,
                            m_counts, m_countsInFrame);
}

LoadDataStrategyHisto::LoadDataStrategyHisto(const double minToF, const double maxToF, const double binWidth)
    : m_binEdges{calculateBinEdges(minToF, maxToF, binWidth)} {
  m_counts.resize(NUM_OF_SPECTRA);
  for (auto &item : m_counts) {
    item.resize(m_binEdges.size() - 1);
  }
  m_countsInFrame = m_counts;
}

/**
 * @brief Add an event to the event list.
 *
 * @param minToF The minimum ToF value to consider
 * @param maxToF The maximum ToF value to consider
 * @param tof The time of flight associated witht he evenr
 * @param binWidth The width of each histogram bin
 * @param pixel The detector pixel index.
 */
void LoadDataStrategyEvent::addEvent(double &minToF, double &maxToF, const double tof, const double binWidth,
                                     const size_t pixel) {
  (void)binWidth;
  if (tof > maxToF) {
    maxToF = tof;
  }
  if (tof < minToF) {
    minToF = tof;
  }
  m_eventsInFrame[pixel].addEventQuickly(Types::Event::TofEvent(tof));
}

/**
 * @brief Add a completed frame to the event list
 *
 * @param rawFrames The number of raw frames read from file
 * @param goodFrames The number of frames that fulfill good quality criteria
 * @param eventCountInFrame The event counts in current frame
 * @param minEventsReq The minimum events required to be considered a good frame
 * @param maxEventsReq The maximum events required to be considered a good frame
 * @param frameEventCounts Event count by frame
 */
void LoadDataStrategyEvent::addFrame(int &rawFrames, int &goodFrames, const int eventCountInFrame,
                                     const int minEventsReq, const int maxEventsReq, MantidVec &frameEventCounts) {
  addFrameToOutputWorkspace(rawFrames, goodFrames, eventCountInFrame, minEventsReq, maxEventsReq, frameEventCounts,
                            m_events, m_eventsInFrame);
}

LoadDataStrategyEvent::LoadDataStrategyEvent() {
  m_events.resize(NUM_OF_SPECTRA);
  m_eventsInFrame.resize(NUM_OF_SPECTRA);
}

} // namespace Mantid::DataHandling
