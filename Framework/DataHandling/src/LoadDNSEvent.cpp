﻿#include "MantidDataHandling/LoadDNSEvent.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
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
#include "MantidKernel/System.h"

#include <stdexcept>
#include <vector>

#include <chrono>
#include <iostream>

#define USE_PARALLELISM true

namespace {

/// coverts data into a hex string
template <typename T> std::string n2hexstr(T const &w, size_t sizeofw = sizeof(T), bool useSpacers = false) {
  static const char *digits = "0123456789ABCDEF";
  const size_t charsPerByte = useSpacers ? 3 : 2;
  const size_t stringLength = (sizeofw - 1) * charsPerByte + 2;
  std::string result(stringLength, '_');

  uint8_t const *const wtmp_p = reinterpret_cast<uint8_t const *const>(&w);

  // auto wtmp = *wtmp_p;
  for (size_t i = 0; i < sizeofw; i++) {
    const auto j = i * charsPerByte;
    result[j] = digits[(wtmp_p[i] & 0xF0) >> 4];
    result[j + 1] = digits[wtmp_p[i] & 0x0F];
  }
  return result;
}

} // namespace

typedef std::array<uint8_t, 8> separator_t;
static constexpr separator_t header_sep{0x00, 0x00, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF};
static constexpr separator_t block_sep = {0x00, 0x00, 0xFF, 0xFF, 0x55, 0x55, 0xAA, 0xAA};   // 0xAAAA5555FFFF0000; //
static constexpr separator_t closing_sig = {0xFF, 0xFF, 0xAA, 0xAA, 0x55, 0x55, 0x00, 0x00}; // 0x00005555AAAAFFFF; //

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadDNSEvent)

const std::string LoadDNSEvent::INSTRUMENT_NAME = "DNS-PSD";
const unsigned MAX_BUFFER_BYTES_SIZE = 1500;      // maximum buffer size in data file
const unsigned DETECTOR_PIXEL_COUNT = 1024 * 128; // number of pixels in detector

void LoadDNSEvent::init() {
  /// Initialise the properties

  const std::vector<std::string> exts{".mdat"};
  declareProperty(std::make_unique<FileProperty>("InputFile", "", FileProperty::Load, exts),
                  "A DNS mesydaq listmode event datafile.");

  declareProperty<uint32_t>("ChopperChannel", 2u, std::make_shared<BoundedValidator<uint32_t>>(0, 4),
                            "The Chopper Channel (0 to 4)", Kernel::Direction::Input);

  // declareProperty<uint32_t>(
  // "MonitorChannel", 1u,
  // std::make_shared<BoundedValidator<uint32_t>>(0, 4),
  // "The Monitor Channel (0 to 4)", Kernel::Direction::Input);

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
  const size_t DUMMY_SIZE = 42;
  EventWorkspace_sptr outputWS = std::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", DETECTOR_PIXEL_COUNT, DUMMY_SIZE, DUMMY_SIZE));
  outputWS->switchEventType(Mantid::API::EventType::TOF);
  outputWS->getAxis(0)->setUnit("TOF");
  outputWS->setYUnit("Counts");
  runLoadInstrument(INSTRUMENT_NAME, outputWS);

  // loadProperties:
  const std::string fileName = getPropertyValue("InputFile");
  chopperChannel = static_cast<uint32_t>(getProperty("ChopperChannel"));
  // monitorChannel = static_cast<uint32_t>(getProperty("MonitorChannel"));
  discardPreChopperEvents = static_cast<bool>(getProperty("DiscardPreChopperEvents"));
  setBinBoundary = static_cast<bool>(getProperty("SetBinBoundary"));

  // const auto instrParamMonitorChannels =
  //    outputWS->instrumentParameters().getType<int>("monitor", "channel");

  if (chopperChannel == 0) {
    const auto instrumentParametersChopperChannels =
        outputWS->instrumentParameters().getType<int>("chopper", "channel");
    if (!instrumentParametersChopperChannels.empty()) {
      chopperChannel = static_cast<uint32_t>(instrumentParametersChopperChannels.at(0));
    } else {
      g_log.error() << "The instrument definition is missing a chopper channel."
                       " You must specify it manually."
                    << std::endl;
    }
  }

  // if (monitorChannel == 0) {
  // const auto instrumentParametersMonitorChannels =
  // outputWS->instrumentParameters().getType<int>("monitor", "channel");
  // if (!instrumentParametersMonitorChannels.empty()) {
  // monitorChannel =
  // static_cast<uint32_t>(instrumentParametersMonitorChannels.at(0));
  // } else {
  // g_log.error() << "The instrument definition is missing a monitor channel."
  // " You must specify it manually."
  // << std::endl;
  // }
  // }

  // g_log.notice() << "ChopperChannel: " << chopperChannel
  // << " MonitorChannel: " << monitorChannel << std::endl;

  FileByteStream file(static_cast<std::string>(fileName), endian::big);

  auto finalEventAccumulator = parse_File(file, fileName);
  populate_EventWorkspace(outputWS, finalEventAccumulator);
  if (setBinBoundary) {
    outputWS->setAllX({0, outputWS->getEventXMax()});
  }
  setProperty("OutputWorkspace", outputWS);
}

template <typename Vector, typename _Compare> void sortVector(Vector &v, _Compare comp) {
  std::sort(v.begin(), v.end(), comp);
}

void LoadDNSEvent::populate_EventWorkspace(EventWorkspace_sptr eventWS, EventAccumulator &finalEventAccumulator) {
  static const unsigned EVENTS_PER_PROGRESS = 100;
  // The number of steps depends on the type of input file
  Progress progress(this, 0.0, 1.0, finalEventAccumulator.neutronEvents.size() / EVENTS_PER_PROGRESS);

  // Sort reversed (latest event first, most early event last):
  sortVector(finalEventAccumulator.triggerEvents, [](auto l, auto r) { return l.timestamp > r.timestamp; });

  std::atomic<uint64_t> oversizedChanelIndexCounterA(0);
  std::atomic<uint64_t> oversizedPosCounterA(0);

  PARALLEL_FOR_IF(Kernel::threadSafe(*eventWS) && USE_PARALLELISM)
  for (int j = 0; j < static_cast<int>(finalEventAccumulator.neutronEvents.size()); ++j) {
    // uint64_t chopperTimestamp = 0;
    uint64_t oversizedChanelIndexCounter = 0;
    uint64_t oversizedPosCounter = 0;
    uint64_t i = 0;
    const auto wsIndex = j;
    auto &eventList = finalEventAccumulator.neutronEvents[j];
    if (eventList.size() != 0) {
      std::sort(eventList.begin(), eventList.end(), [](auto l, auto r) { return l.timestamp < r.timestamp; });
    }

    auto chopperIt = finalEventAccumulator.triggerEvents.cbegin();

    auto &spectrum = eventWS->getSpectrum(wsIndex);
    // PARALLEL_START_INTERUPT_REGION

    for (const auto &event : eventList) {
      i++;
      if (i % EVENTS_PER_PROGRESS == 0) {
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
      if ((chopperTimestamp == 0) && discardPreChopperEvents) {
        // throw away events before first chopper trigger
        continue;
      }
      spectrum.addEventQuickly(Types::Event::TofEvent(double(event.timestamp - chopperTimestamp) / 10.0));
    }

    // PARALLEL_END_INTERUPT_REGION
    oversizedChanelIndexCounterA += oversizedChanelIndexCounter;
    oversizedPosCounterA += oversizedPosCounter;
    // PARALLEL_CHECK_INTERUPT_REGION
  }

  if (oversizedChanelIndexCounterA > 0) {
    g_log.warning() << "Bad chanel indices: " << oversizedChanelIndexCounterA << std::endl;
  }
  if (oversizedPosCounterA > 0) {
    g_log.warning() << "Bad position values: " << oversizedPosCounterA << std::endl;
  }
}

void LoadDNSEvent::runLoadInstrument(std::string instrumentName, EventWorkspace_sptr &eventWS) {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", instrumentName);
    g_log.debug() << "InstrumentName" << instrumentName << '\n';
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", eventWS);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));

    loadInst->execute();
  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

namespace {
template <typename Iterable>
constexpr std::map<std::remove_reference_t<decltype(*std::declval<decltype(std::declval<Iterable>().cbegin())>())>,
                   size_t>
buildSkipTable(const Iterable &iterable) {
  std::map<std::remove_reference_t<decltype(*std::declval<decltype(std::declval<Iterable>().cbegin())>())>, size_t>
      skipTable;

  size_t i = iterable.size();
  for (auto c : iterable) {
    i--;
    auto &cPos = skipTable[c];
    cPos = cPos == 0 ? iterable.size() : cPos;
    cPos = i == 0 ? static_cast<size_t>(cPos) : i;
  }
  return skipTable;
}

template <typename Iterable> const std::vector<uint8_t> buildSkipTable2(const Iterable &iterable) {
  std::vector<uint8_t> skipTable(256, static_cast<uint8_t>(iterable.size()));

  size_t i = iterable.size();
  for (auto c : iterable) {
    i--;
    auto &cPos = skipTable[c];
    cPos = i == 0 ? cPos : static_cast<uint8_t>(i);
  }
  return skipTable;
}

} // namespace

std::vector<uint8_t> LoadDNSEvent::parse_Header(FileByteStream &file) {
  // using Boyer-Moore String Search:
  static constexpr std::array<uint8_t, 8> header_sep{0x00, 0x00, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF};
  static const auto skipTable = buildSkipTable(header_sep);

  // search for header_sep and store actual header:
  std::vector<uint8_t> header;

  std::array<uint8_t, header_sep.size()> current_window;
  file.readRaw(current_window);
  try {
    while (!file.eof()) {
      if (current_window == header_sep) {
        return header;
      } else {
        auto iter = skipTable.find(*current_window.rbegin());
        size_t skip_length = (iter == skipTable.end()) ? header_sep.size() : iter->second;

        const auto orig_header_size = header.size();
        header.resize(header.size() + skip_length);
        const auto win_data = current_window.data();
        std::copy(win_data, win_data + skip_length, header.data() + orig_header_size);

        const std::array<uint8_t, header_sep.size()> orig_window = current_window;
        file.readRaw(current_window, skip_length);
        std::copy(orig_window.data() + skip_length, orig_window.data() + header_sep.size(), win_data);
      }
    }

  } catch (std::ifstream::failure &) {
    return header;
  }

  return header;
}

std::vector<std::vector<uint8_t>> LoadDNSEvent::split_File(FileByteStream &file, const unsigned maxChunckCount) {
  static const auto skipTable = buildSkipTable2(block_sep);

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

      while (*windowAsArray != block_sep) { // (!file.eof() ) {
        size_t skip_length = skipTable[current_window[windowSize - 1]];

        const auto orig_data_size = data.size();
        data.resize(orig_data_size + skip_length);

        // accomodate for possible relocation of vector...:
        current_window = (&data.back() - windowSize + 1);

        windowAsArray = reinterpret_cast<std::array<uint8_t, windowSize> *>(current_window);
        file.readRaw(current_window[windowSize - skip_length], skip_length);
      }
    } catch (std::ifstream::failure &) {
      return result;
    }
  } // while
  return result;
}

namespace {

template <typename V1, typename V2> bool startsWith(const V1 &sequence, const V2 &subSequence) {
  return std::equal(std::begin(subSequence), std::end(subSequence), std::begin(sequence));
}

template <typename V1, typename V2> bool endsWith(const V1 &sequence, const V2 &subSequence) {
  auto dist = std::distance(std::begin(subSequence), std::end(subSequence));
  return std::equal(begin(subSequence), end(subSequence), end(sequence) - dist);
}

} // namespace

LoadDNSEvent::EventAccumulator LoadDNSEvent::parse_File(FileByteStream &file, const std::string fileName) {
  // File := Header Body
  std::vector<uint8_t> header = parse_Header(file);

  // check it is actually a mesytec psd listmode file:
  if (!startsWith(header, std::string("mesytec psd listmode data"))) {
    g_log.error() << "This seems not to be a mesytec psd listmode file: " << fileName;
    throw Exception::FileError("This seems not to be a mesytec psd listmode file: ", fileName);
  }

  // Parse actual data:
  const int threadCount = USE_PARALLELISM ? PARALLEL_GET_MAX_THREADS : 1;
  // Split File:
  std::vector<std::vector<uint8_t>> filechuncks = split_File(file, threadCount);
  g_log.debug() << "filechuncks count = " << filechuncks.size() << std::endl;

  std::vector<EventAccumulator> eventAccumulators(filechuncks.size());
  for (auto &evtAcc : eventAccumulators) {
    evtAcc.neutronEvents.resize(DETECTOR_PIXEL_COUNT);
  }

  // parse individual file chuncks:
  PARALLEL_FOR_IF(USE_PARALLELISM)
  for (int i = 0; i < static_cast<int>(filechuncks.size()); ++i) {
    auto filechunck = filechuncks[static_cast<size_t>(i)];
    g_log.debug() << "filechunck.size() = " << filechunck.size() << std::endl;
    auto vbs = VectorByteStream(filechunck, file.endianess());
    parse_BlockList(vbs, eventAccumulators[static_cast<size_t>(i)]);
  }

  EventAccumulator finalEventAccumulator;
  finalEventAccumulator.neutronEvents.resize(DETECTOR_PIXEL_COUNT);

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

  // parse_BlockList(file, _eventAccumulator);
  // parse_EndSignature(file);

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
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator) +
                             std::string("expected 0x") + n2hexstr(block_sep));
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
  uint16_t ts1;
  uint16_t ts2;
  uint16_t ts3;
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
  // g_log.debug() << int(header.bufferLength)  << " "
  //              << int(header.headerLength)  << " "
  //              << int(header.bufferNumber)  << " "
  //             << int(header.runId)  << " "
  //             << int(header.runId)  << " "
  //             << int(header.mcpdId)  << " "
  //             << int(header.deviceStatus)  << " "
  //             << ts1  << " "
  //             << ts2  << " "
  //             << ts3  << " "
  //             << header.timestamp  << " "
  //             << std::endl;
  file.skip<24>();
  return header;
}

void LoadDNSEvent::parse_andAddEvent(VectorByteStream &file, const LoadDNSEvent::BufferHeader &bufferHeader,
                                     LoadDNSEvent::EventAccumulator &eventAccumulator) {
  CompactEvent event = {};
  uint16_t data1;
  uint16_t data2;
  uint16_t data3;
  uint64_t data;
  event_id_e eventId;
  file.read<2>(data1);
  file.read<2>(data2);
  file.read<2>(data3);
  // 48 bit event is 3 2-byte MSB words ordered LSB
  data = (uint64_t)data3 << 32 | (uint64_t)data2 << 16 | data1;
  eventId = static_cast<event_id_e>((data >> 47) & 0b1);
  switch (eventId) {
  case event_id_e::TRIGGER: {
    uint8_t trigId;
    uint8_t dataId;
    trigId = (data >> 44) & 0b111;    // 3 bit
    dataId = (data >> 40) & 0b1111;   // 4 bit
    event.timestamp = data & 0x7ffff; // 19bit
    event.timestamp += bufferHeader.timestamp;
    // g_log.debug() << "trigger " << trigId
    //              << " data_id " << dataId
    //             << " timestamp " << event.timestamp << std::endl;
    // trigId = 7 is register change and
    // dataId 0-3 corresponds to chopperchannel 1-4 in the gui
    if ((dataId != chopperChannel - 1) || (trigId != 7)) {
      return;
    }
    eventAccumulator.triggerEvents.push_back(event);
  } break;
  case event_id_e::NEUTRON: {
    uint16_t position;
    uint8_t channel;
    channel = (data >> 39) & 0b11111111; // 5bit
    position = (data >> 19) & 0x3ff;     // 10bit
    event.timestamp = (data & 0x7ffff);  // 19bit
    event.timestamp += bufferHeader.timestamp;
    channel |= static_cast<uint8_t>(bufferHeader.mcpdId << 6u);
    // g_log.debug() << "channel " << int(channel)
    //              << " position " << position
    //              << " timestamp " << event.timestamp
    //              << std::endl;
    const size_t wsIndex = getWsIndex(channel, position);
    eventAccumulator.neutronEvents[wsIndex].push_back(event);
  } break;
  default:
    // Panic!!!!
    g_log.error() << "unknown event id " << eventId << "\n";
    break;
  }
}

void LoadDNSEvent::parse_EndSignature(FileByteStream &file) {
  auto separator = file.readRaw(separator_t());
  if (separator != closing_sig) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator) +
                             std::string("expected 0x") + n2hexstr(closing_sig));
  }
}

} // namespace DataHandling
} // namespace Mantid
