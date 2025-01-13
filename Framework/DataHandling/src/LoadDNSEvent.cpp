// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadDNSEvent.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include <stdexcept>
#include <vector>

#include <chrono>
#include <iostream>

using separator_t = std::array<uint8_t, 8>;
static constexpr separator_t header_sep{0x00, 0x00, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF};
static constexpr separator_t block_sep = {0x00, 0x00, 0xFF, 0xFF, 0x55, 0x55, 0xAA, 0xAA};   // 0xAAAA5555FFFF0000; //
static constexpr separator_t closing_sig = {0xFF, 0xFF, 0xAA, 0xAA, 0x55, 0x55, 0x00, 0x00}; // 0x00005555AAAAFFFF; //

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {

template <typename V1, typename V2> bool startsWith(const V1 &sequence, const V2 &subSequence) {
  return std::equal(std::begin(subSequence), std::end(subSequence), std::begin(sequence));
}

} // namespace

namespace Mantid::DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadDNSEvent)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadDNSEvent::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  if (extn != ".mdat")
    return 0;
  auto &file = descriptor.data();

  std::string fileline;
  std::getline(file, fileline);
  if (fileline.find("mesytec psd listmode data") != std::string::npos) {
    return 80;
  } else
    return 0;
}

const unsigned int MAX_BUFFER_BYTES_SIZE = 1500; // maximum buffer size in data file
const unsigned int PIXEL_PER_TUBE = 1024;        // maximum buffer size in data file

void LoadDNSEvent::init() {
  /// Initialise the properties

  const std::vector<std::string> exts{".mdat"};
  declareProperty(std::make_unique<FileProperty>("InputFile", "", FileProperty::Load, exts),
                  "A DNS mesydaq listmode event datafile.");

  declareProperty<uint32_t>("ChopperChannel", 2u, std::make_shared<BoundedValidator<uint32_t>>(1, 4),
                            "The Chopper Channel (1 to 4)", Kernel::Direction::Input);

  declareProperty<uint32_t>("NumberOfTubes", 128, std::make_shared<BoundedValidator<uint32_t>>(1, 128),
                            "The number of tubes, each tube has 1024 pixels (1 to 128)", Kernel::Direction::Input);

  declareProperty<bool>("DiscardPreChopperEvents", true, std::make_shared<BoundedValidator<bool>>(0, 1),
                        "Discards events before first chopper trigger (turn off for elastic)",
                        Kernel::Direction::Input);

  declareProperty<bool>("SetBinBoundary", true, std::make_shared<BoundedValidator<bool>>(0, 1),
                        "Sets all bin boundaries to include all events (can be turned off to save time).",
                        Kernel::Direction::Input);

  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::EventWorkspace>>("OutputWorkspace", "", Direction::Output),
      "The name of the output workspace.");
}

/// Run the algorithm
void LoadDNSEvent::exec() {
  // loadProperties:
  const std::string fileName = getPropertyValue("InputFile");
  m_chopperChannel = static_cast<uint32_t>(getProperty("ChopperChannel"));
  m_discardPreChopperEvents = getProperty("DiscardPreChopperEvents");
  m_setBinBoundary = getProperty("SetBinBoundary");
  m_detectorPixelCount = static_cast<uint32_t>(getProperty("NumberOfTubes")) * PIXEL_PER_TUBE;

  // create workspace
  EventWorkspace_sptr outputWS = std::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", m_detectorPixelCount, 1, 1));
  outputWS->switchEventType(Mantid::API::EventType::TOF);
  outputWS->getAxis(0)->setUnit("TOF");
  outputWS->setYUnit("Counts");

  // g_log.notice() << "ChopperChannel: " << m_chopperChannel << std::endl;

  FileByteStream file(static_cast<std::string>(fileName));

  auto finalEventAccumulator = parse_File(file, fileName);
  populate_EventWorkspace(outputWS, finalEventAccumulator);
  if (m_setBinBoundary && (outputWS->getNumberEvents() > 0)) {
    outputWS->setAllX({0, outputWS->getEventXMax()});
  }
  setProperty("OutputWorkspace", outputWS);
}

void LoadDNSEvent::populate_EventWorkspace(EventWorkspace_sptr &eventWS, EventAccumulator &finalEventAccumulator) {
  static const unsigned EVENTS_PER_PROGRESS = 100;
  // The number of steps depends on the type of input file
  Progress progress(this, 0.0, 1.0, finalEventAccumulator.neutronEvents.size() / EVENTS_PER_PROGRESS);

  // Sort reversed (latest event first, most early event last):
  std::sort(finalEventAccumulator.triggerEvents.begin(), finalEventAccumulator.triggerEvents.end(),
            [](auto l, auto r) { return l.timestamp > r.timestamp; });

  std::atomic<unsigned int> oversizedChanelIndexCounterA(0);
  std::atomic<unsigned int> oversizedPosCounterA(0);

  PARALLEL_FOR_IF(Kernel::threadSafe(*eventWS))
  for (int eventIndex = 0; eventIndex < static_cast<int>(finalEventAccumulator.neutronEvents.size()); ++eventIndex) {
    // uint64_t chopperTimestamp = 0;
    unsigned int oversizedChanelIndexCounter = 0;
    unsigned int oversizedPosCounter = 0;
    const auto wsIndex = eventIndex;
    auto &eventList = finalEventAccumulator.neutronEvents[eventIndex];
    if (eventList.size() != 0) {
      std::sort(eventList.begin(), eventList.end(), [](auto l, auto r) { return l.timestamp < r.timestamp; });
    }

    auto chopperIt = finalEventAccumulator.triggerEvents.cbegin();

    auto &spectrum = eventWS->getSpectrum(wsIndex);
    PARALLEL_START_INTERRUPT_REGION

    uint64_t numProcessed = 0;
    for (const auto &event : eventList) {
      numProcessed++;
      if (numProcessed % EVENTS_PER_PROGRESS == 0) {
        progress.report();
        if (this->getCancel()) {
          throw CancelException();
        }
      }

      chopperIt =
          std::lower_bound(finalEventAccumulator.triggerEvents.cbegin(), finalEventAccumulator.triggerEvents.cend(),
                           event.timestamp, [](auto l, auto r) { return l.timestamp > r; });
      const uint64_t chopperTimestamp = chopperIt != finalEventAccumulator.triggerEvents.cend()
                                            ? chopperIt->timestamp
                                            : 0; // before first chopper trigger
      if ((chopperTimestamp == 0) && m_discardPreChopperEvents) {
        // throw away events before first chopper trigger
        continue;
      }
      spectrum.addEventQuickly(Types::Event::TofEvent(double(event.timestamp - chopperTimestamp) / 10.0));
    }

    PARALLEL_END_INTERRUPT_REGION
    oversizedChanelIndexCounterA += oversizedChanelIndexCounter;
    oversizedPosCounterA += oversizedPosCounter;
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (oversizedChanelIndexCounterA > 0) {
    g_log.warning() << "Bad chanel indices: " << oversizedChanelIndexCounterA << std::endl;
  }
  if (oversizedPosCounterA > 0) {
    g_log.warning() << "Bad position values: " << oversizedPosCounterA << std::endl;
  }
}

std::vector<uint8_t> LoadDNSEvent::parse_Header(FileByteStream &file) {
  // using Boyer-Moore String Search:

  // search for header_sep and store actual header:
  std::vector<uint8_t> header;

  std::array<uint8_t, header_sep.size()> current_window;
  file.readRaw(current_window);
  while (!file.eof()) {
    if (current_window == header_sep) {
      return header;
    } else {
      const auto orig_header_size = header.size();
      header.resize(header.size() + 1);
      const auto win_data = current_window.data();
      std::copy(win_data, win_data + 1, header.data() + orig_header_size);
      const std::array<uint8_t, header_sep.size()> orig_window = current_window;
      file.readRaw(current_window, 1);
      std::copy(orig_window.data() + 1, orig_window.data() + header_sep.size(), win_data);
    }
  }
  return header;
}

std::vector<std::vector<uint8_t>> LoadDNSEvent::split_File(FileByteStream &file, const unsigned maxChunckCount) {

  const uint64_t minChunckSize = MAX_BUFFER_BYTES_SIZE;
  const uint64_t chunckSize = std::max(minChunckSize, file.fileSize() / maxChunckCount);

  std::vector<std::vector<uint8_t>> result;

  while (!file.eof()) {
    result.push_back(std::vector<uint8_t>());
    // read a big chunck of file:
    auto &data = result.back();
    data.resize(chunckSize);
    try {
      file.readRaw(*data.begin(), chunckSize);
    } catch (std::ifstream::failure &) {
      data.resize(file.gcount());
      return result;
    }

    // search for a block_separator, and append everything up to it :
    static const auto windowSize = block_sep.size();
    uint8_t *current_window = nullptr;
    std::array<uint8_t, windowSize> *windowAsArray =
        reinterpret_cast<std::array<uint8_t, windowSize> *>(current_window);

    try {
      data.resize(data.size() + windowSize);
      // accomodate for possible relocation of vector...:
      current_window = &*(data.end() - windowSize);
      windowAsArray = reinterpret_cast<std::array<uint8_t, windowSize> *>(current_window);
      file.readRaw(current_window[0], windowSize);

      while ((*windowAsArray != block_sep) && (!file.eof())) {
        const auto orig_data_size = data.size();
        data.resize(orig_data_size + 1);
        // accomodate for possible relocation of vector...:
        current_window = (&data.back() - windowSize + 1);
        windowAsArray = reinterpret_cast<std::array<uint8_t, windowSize> *>(current_window);
        file.readRaw(current_window[windowSize - 1], 1);
      }
    } catch (std::ifstream::failure &) {
      return result;
    }
  } // while
  return result;
}

LoadDNSEvent::EventAccumulator LoadDNSEvent::parse_File(FileByteStream &file, const std::string &fileName) {
  // File := Header Body
  std::vector<uint8_t> header = parse_Header(file);

  // check it is actually a mesytec psd listmode file:
  if (!startsWith(header, std::string("mesytec psd listmode data"))) {
    g_log.error() << "This seems not to be a mesytec psd listmode file: " << fileName;
    throw Exception::FileError("This seems not to be a mesytec psd listmode file: ", fileName);
  }

  // Parse actual data:
  const int threadCount = PARALLEL_GET_MAX_THREADS;
  // Split File:
  std::vector<std::vector<uint8_t>> filechuncks = split_File(file, threadCount);
  g_log.debug() << "filechuncks count = " << filechuncks.size() << std::endl;

  std::vector<EventAccumulator> eventAccumulators(filechuncks.size());
  for (auto &evtAcc : eventAccumulators) {
    evtAcc.neutronEvents.resize(m_detectorPixelCount);
  }

  // parse individual file chuncks:
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(filechuncks.size()); ++i) {
    auto filechunck = filechuncks[static_cast<size_t>(i)];
    g_log.debug() << "filechunck.size() = " << filechunck.size() << std::endl;
    auto vbs = VectorByteStream(filechunck);
    parse_BlockList(vbs, eventAccumulators[static_cast<size_t>(i)]);
  }

  EventAccumulator finalEventAccumulator;
  finalEventAccumulator.neutronEvents.resize(m_detectorPixelCount);

  // combine eventAccumulators:

  // combine triggerEvents:
  for (const auto &v : eventAccumulators) {
    finalEventAccumulator.triggerEvents.insert(finalEventAccumulator.triggerEvents.end(), v.triggerEvents.begin(),
                                               v.triggerEvents.end());
  }

  // combine neutronEvents:
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(finalEventAccumulator.neutronEvents.size()); ++i) {
    auto &allNeutronEvents = finalEventAccumulator.neutronEvents[static_cast<size_t>(i)];
    for (const auto &v : eventAccumulators) {
      allNeutronEvents.insert(allNeutronEvents.end(), v.neutronEvents[static_cast<size_t>(i)].begin(),
                              v.neutronEvents[static_cast<size_t>(i)].end());
    }
  }

  return finalEventAccumulator;
}

void LoadDNSEvent::parse_BlockList(VectorByteStream &file, EventAccumulator &eventAccumulator) {
  // BlockList := DataBuffer BlockListTrail
  while (!file.eof() && file.peek() != 0xFF) {
    parse_Block(file, eventAccumulator);
  }
}

void LoadDNSEvent::parse_Block(VectorByteStream &file, EventAccumulator &eventAccumulator) {
  // Block := DataBufferHeader DataBuffer
  parse_DataBuffer(file, eventAccumulator);
  parse_BlockSeparator(file);
}

void LoadDNSEvent::parse_BlockSeparator(VectorByteStream &file) {
  auto separator = file.readRaw(separator_t());
  if (separator != block_sep) {
    throw std::runtime_error(std::string("File Integrety LOST. 0x"));
  }
}

void LoadDNSEvent::parse_DataBuffer(VectorByteStream &file, EventAccumulator &eventAccumulator) {
  const auto bufferHeader = parse_DataBufferHeader(file);

  const uint16_t dataLength = uint16_t(bufferHeader.bufferLength - 21);
  const auto event_count = dataLength / 3;

  for (uint16_t i = 0; i < event_count; i++) {
    parse_andAddEvent(file, bufferHeader, eventAccumulator);
  }
}

LoadDNSEvent::BufferHeader LoadDNSEvent::parse_DataBufferHeader(VectorByteStream &file) {
  uint16_t ts1 = 0;
  uint16_t ts2 = 0;
  uint16_t ts3 = 0;
  BufferHeader header = {};
  file.read<2>(header.bufferLength);
  file.read<2>(header.bufferVersion);
  file.read<2>(header.headerLength);
  file.read<2>(header.bufferNumber);
  file.read<2>(header.runId);
  file.read<1>(header.mcpdId);
  file.read<1>(header.deviceStatus);
  file.read<2>(ts1);
  file.read<2>(ts2);
  file.read<2>(ts3);
  // 48 bit timestamp is 3 2-byte MSB words ordered LSB
  header.timestamp = (uint64_t)ts3 << 32 | (uint64_t)ts2 << 16 | ts1;
  file.skip<24>();
  return header;
}

LoadDNSEvent::TriggerEvent LoadDNSEvent::processTrigger(const uint64_t &data,
                                                        const LoadDNSEvent::BufferHeader &bufferHeader) {
  TriggerEvent triggerEvent = {};
  uint8_t trigId = 0;
  uint8_t dataId = 0;
  trigId = (data >> 44) & 0b111;                 // 3 bit
  dataId = (data >> 40) & 0b1111;                // 4 bit
  triggerEvent.event.timestamp = data & 0x7ffff; // 19bit
  triggerEvent.event.timestamp += bufferHeader.timestamp;
  triggerEvent.isChopperTrigger = ((dataId == m_chopperChannel - 1) && (trigId == 7));
  return triggerEvent;
}

LoadDNSEvent::NeutronEvent LoadDNSEvent::processNeutron(const uint64_t &data,
                                                        const LoadDNSEvent::BufferHeader &bufferHeader) {
  NeutronEvent neutronEvent = {};
  uint16_t position = 0;
  uint8_t modid = 0;
  uint8_t slotid = 0;
  modid = (data >> 44) & 0b111;                    // 3bit
  slotid = (data >> 39) & 0b11111;                 // 5bit
  position = (data >> 19) & 0x3ff;                 // 10bit
  neutronEvent.event.timestamp = (data & 0x7ffff); // 19bit
  neutronEvent.event.timestamp += bufferHeader.timestamp;
  neutronEvent.wsIndex = (bufferHeader.mcpdId << 6u | modid << 3u | slotid) << 10u | position;
  return neutronEvent;
}

void LoadDNSEvent::parse_andAddEvent(VectorByteStream &file, const LoadDNSEvent::BufferHeader &bufferHeader,
                                     LoadDNSEvent::EventAccumulator &eventAccumulator) {
  uint16_t data1 = 0;
  uint16_t data2 = 0;
  uint16_t data3 = 0;
  uint64_t data = 0;
  event_id_e eventId;
  file.read<2>(data1);
  file.read<2>(data2);
  file.read<2>(data3);
  // 48 bit event is 3 2-byte MSB words ordered LSB
  data = (uint64_t)data3 << 32 | (uint64_t)data2 << 16 | data1;
  eventId = static_cast<event_id_e>((data >> 47) & 0b1);
  switch (eventId) {
  case event_id_e::TRIGGER: {
    const auto triggerEvent = processTrigger(data, bufferHeader);
    if (triggerEvent.isChopperTrigger) {
      eventAccumulator.triggerEvents.push_back(triggerEvent.event);
    }
  } break;
  case event_id_e::NEUTRON: {
    const auto neutronEvent = processNeutron(data, bufferHeader);
    eventAccumulator.neutronEvents[neutronEvent.wsIndex].push_back(neutronEvent.event);
  } break;
  default:
    g_log.error() << "unknown event id " << eventId << "\n";
    break;
  }
}

void LoadDNSEvent::parse_EndSignature(FileByteStream &file) {
  auto separator = file.readRaw(separator_t());
  if (separator != closing_sig) {
    throw std::runtime_error(std::string("File Integrety LOST. 0x"));
  }
}

} // namespace Mantid::DataHandling
