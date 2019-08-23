// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadNGEM.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <sys/stat.h>

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadNGEM)

/**
 * @brief Byte swap a 64 bit word as the nGEM detector outputs in big endian
 * format. So must be swapped to have correct values on x86 and x64
 * architectures.
 *
 * @param word The 64 bit word to swap.
 * @return uint64_t The swapped word.
 */
uint64_t LoadNGEM::swapUint64(uint64_t word) {
  word = ((word << 8) & 0xFF00FF00FF00FF00ULL) |
         ((word >> 8) & 0x00FF00FF00FF00FFULL);
  word = ((word << 16) & 0xFFFF0000FFFF0000ULL) |
         ((word >> 16) & 0x0000FFFF0000FFFFULL);
  return (word << 32) | (word >> 32);
}

/**
 * @brief The confidence that a file can be loaded.
 *
 * @param descriptor A description of the file.
 * @return int The level of certainty that the file can be loaded with this
 * loader.
 */
int LoadNGEM::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension() == ".edb") {
    return 100;
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
  declareProperty(
      std::make_unique<Mantid::API::FileProperty>(
          "Filename", "", Mantid::API::FileProperty::Load, extentions),
      "The name of the nGEM file to load. Selecting multiple files will "
      "combine them into one workspace.");
  // Output workspace
  declareProperty(
      std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The output workspace");

  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);

  auto mustBePositiveDbl =
      boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  // Bin Width
  declareProperty("BinWidth", 10.0, mustBePositiveDbl,
                  "The width of the time bins in the output.");

  declareProperty("MinEventsPerFrame", 0, mustBePositive,
                  "The minimum number of events required in a frame before a "
                  "it is considered 'good'.");
  declareProperty("MaxEventsPerFrame", Mantid::EMPTY_INT(), mustBePositive,
                  "The maximum number of events allowed in a frame to be "
                  "considered 'good'.");
  declareProperty(
      std::make_unique<Mantid::Kernel::PropertyWithValue<bool>>(
          "GenerateEventsPerFrame", false, Mantid::Kernel::Direction::Input),
      "Generate a workspace to show the number of events captured by each "
      "frame. (optional, default False).");
}

/**
 * @brief Execute the algorithm.
 */
void LoadNGEM::exec() {
  progress(0);
  const std::string filename = getPropertyValue("Filename");
  const int NUM_OF_SPECTRA = 16384;
  const int minEventsReq = stoi(getPropertyValue("MinEventsPerFrame"));
  const int maxEventsReq = stoi(getPropertyValue("MaxEventsPerFrame"));

  // Create file reader
  FILE *file = fopen(filename.c_str(), "rb");
  if (file == NULL) {
    g_log.error() << "Invalid input filename.\n";
    throw std::runtime_error("File could not be found.");
  }

  size_t totalNumEvents = verifyFileSize(file) / 16;
  size_t numProcessedEvents = 0;

  EventUnion event, eventBigEndian;
  size_t loadStatus;

  int maxToF = -1;
  int minToF = 2147483647;
  const double BIN_WIDTH(stod(getPropertyValue("BinWidth")));

  int rawFrames = 0;
  int goodFrames = 0;
  std::vector<double> frameEventCounts;
  int eventCountInFrame = 0;

  std::vector<DataObjects::EventList> histograms, histogramsInFrame;
  histograms.resize(NUM_OF_SPECTRA);
  histogramsInFrame.resize(NUM_OF_SPECTRA);
  progress(0.04);

  while (!feof(file)) {
    // Load an event into the variable.
    loadStatus = fread(&eventBigEndian, sizeof(eventBigEndian), 1, file);

    // Check that the load was successful.
    if (loadStatus == 0) {
      g_log.information() << "File loading complete!\n";
      break;
    } else if (loadStatus != 1) {
      g_log.error() << "Error while reading file.";
      throw std::runtime_error("Error while reading file.");
    }

    // Correct for the big endian format.
    correctForBigEndian(eventBigEndian, event);

    if (event.coincidence.check()) { // Check for coincidence event.
      ++eventCountInFrame;
      uint64_t pixel = event.coincidence.getPixel();
      int tof =
          event.coincidence.timeOfFlight / 1000; // Convert to microseconds (us)

      if (tof > maxToF) {
        maxToF = tof;
      } else if (tof < minToF) {
        minToF = tof;
      }
      histogramsInFrame[pixel].addEventQuickly(
          Mantid::Types::Event::TofEvent(tof));

    } else if (event.tZero.check()) { // Check for T0 event.
      if (eventCountInFrame >= minEventsReq &&
          eventCountInFrame <= maxEventsReq) {
        ++rawFrames;
        // Add number of event counts to workspace.
        frameEventCounts.emplace_back(eventCountInFrame);
        ++goodFrames;

        // Add histograms that match parameters to workspace
        for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
          if (histogramsInFrame[i].getNumberEvents() > 0) {
            histograms[i] += histogramsInFrame[i];
            histogramsInFrame[i].clear();
          }
        }
      }
      // Progess Reporting
      numProcessedEvents += eventCountInFrame;
      progress(double(numProcessedEvents) / double(totalNumEvents) / 1.11111);
      eventCountInFrame = 0;
      // Check for cancel flag.
      if (this->getCancel()) {
        return;
      }
    } else { // Catch all other events and notify.
      g_log.warning() << "Unexpected event type loaded.\n";
    }
  }
  // Add the final set of events.
  if (eventCountInFrame >= minEventsReq && eventCountInFrame <= maxEventsReq) {
    frameEventCounts.emplace_back(eventCountInFrame);
    ++goodFrames;
    for (auto i = 0; i < NUM_OF_SPECTRA; ++i) {
      histograms[i] += histogramsInFrame[i];
    }
  }
  ++rawFrames;

  fclose(file);
  progress(0.90);

  // Create and fill main histogram data into an event workspace.
  std::vector<double> xAxis;
  for (auto i = 0; i < (maxToF / BIN_WIDTH); i++) {
    xAxis.push_back(i * BIN_WIDTH);
  }

  m_dataWorkspace = DataObjects::create<DataObjects::EventWorkspace>(
      NUM_OF_SPECTRA,
      Mantid::HistogramData::Histogram(Mantid::HistogramData::BinEdges(xAxis)));

  for (auto spectrumNo = 0u; spectrumNo < histograms.size(); ++spectrumNo) {
    m_dataWorkspace->getSpectrum(spectrumNo) = histograms[spectrumNo];
    m_dataWorkspace->getSpectrum(spectrumNo).setSpectrumNo(spectrumNo + 1);
    m_dataWorkspace->getSpectrum(spectrumNo).setDetectorID(spectrumNo + 1);
  }
  m_dataWorkspace->setAllX(HistogramData::BinEdges{xAxis});
  m_dataWorkspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("TOF");
  m_dataWorkspace->setYUnit("Counts");

  addToSampleLog("raw_frames", rawFrames, m_dataWorkspace);
  addToSampleLog("good_frames", goodFrames, m_dataWorkspace);

  loadInstrument();

  setProperty("OutputWorkspace", m_dataWorkspace);
  if (this->getProperty("GenerateEventsPerFrame")) {
    createCountWorkspace(frameEventCounts);
  }
  progress(1.00);
}

void LoadNGEM::correctForBigEndian(const EventUnion &bigEndian,
                                   EventUnion &smallEndian) {
  smallEndian.splitWord.words[0] = swapUint64(bigEndian.splitWord.words[1]);
  smallEndian.splitWord.words[1] = swapUint64(bigEndian.splitWord.words[0]);
}

void LoadNGEM::addToSampleLog(const std::string &logName,
                              const std::string &logText,
                              DataObjects::EventWorkspace_sptr &ws) {
  Mantid::API::Algorithm_sptr sampLogAlg = createChildAlgorithm("AddSampleLog");
  sampLogAlg->setProperty("Workspace", ws);
  sampLogAlg->setProperty("LogType", "String");
  sampLogAlg->setProperty("LogName", logName);
  sampLogAlg->setProperty("LogText", logText);
  sampLogAlg->executeAsChildAlg();
}

void LoadNGEM::addToSampleLog(const std::string &logName, const int &logNumber,
                              DataObjects::EventWorkspace_sptr &ws) {
  Mantid::API::Algorithm_sptr sampLogAlg = createChildAlgorithm("AddSampleLog");
  sampLogAlg->setProperty("Workspace", ws);
  sampLogAlg->setProperty("LogType", "Number");
  sampLogAlg->setProperty("LogName", logName);
  sampLogAlg->setProperty("LogText", std::to_string(logNumber));
  sampLogAlg->executeAsChildAlg();
}

size_t LoadNGEM::verifyFileSize(FILE *&file) {
  // Check that the file fits into 16 byte sections.
  struct stat fileStatus;
  if (fstat(fileno(file), &fileStatus) != 0 || fileStatus.st_size % 16 != 0) {
    g_log.warning() << "Invalid file size. Data may be corrupted.\n";
  }
  return fileStatus.st_size;
}

/**
 * @brief Create a count workspace to allow for the selection of "good" frames.
 *
 * @param frameEventCounts A Vector of the number of events per frame.
 */
void LoadNGEM::createCountWorkspace(
    const std::vector<double> &frameEventCounts) {
  std::vector<double> xAxisCounts;
  for (auto i = 0u; i <= frameEventCounts.size(); i++) {
    xAxisCounts.push_back(i);
  }

  DataObjects::Workspace2D_sptr countsWorkspace =
      DataObjects::create<DataObjects::Workspace2D>(
          1, Mantid::HistogramData::Histogram(
                 Mantid::HistogramData::BinEdges(xAxisCounts)));

  countsWorkspace->mutableY(0) = frameEventCounts;
  std::string countsWorkspaceName(this->getProperty("OutputWorkspace"));
  countsWorkspaceName.append("_event_counts");
  countsWorkspace->setYUnit("Counts");
  boost::shared_ptr<Kernel::Units::Label> XLabel =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          Kernel::UnitFactory::Instance().create("Label"));
  XLabel->setLabel("Frame");
  countsWorkspace->getAxis(0)->unit() = XLabel;

  this->declareProperty(
      std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          "CountsWorkspace", countsWorkspaceName, Kernel::Direction::Output),
      "Counts of events per frame.");
  progress(1.00);
  this->setProperty("CountsWorkspace", countsWorkspace);
}

void LoadNGEM::loadInstrument() {
  auto loadInstrument = this->createChildAlgorithm("LoadInstrument");
  loadInstrument->setPropertyValue("InstrumentName", "NGEM");
  loadInstrument->setProperty<Mantid::API::MatrixWorkspace_sptr>(
      "Workspace", m_dataWorkspace);
  loadInstrument->setProperty("RewriteSpectraMap",
                              Mantid::Kernel::OptionalBool(false));
  loadInstrument->execute();
}

} // namespace DataHandling
} // namespace Mantid