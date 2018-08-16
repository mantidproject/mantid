#include <sstream>

#include "MantidDataHandling/LoadDNSEvent.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"

#include <boost/range/combine.hpp>
#include <ios>
#include <fstream>
#include <vector>
#include <map>
#include <stdexcept>
#include <ostream>

#include <chrono>
#include <sstream>
#include <iostream>

//#define LOG_INFORMATION(a) g_log.notice() << a;
#define LOG_INFORMATION(a) //g_log.information()##<<##a;

template<typename TimeT = std::chrono::milliseconds>
struct measure
{
    template<typename F, typename ...Args>
    static typename TimeT::rep execution(F&& func, Args&&... args)
    {
        auto start = std::chrono::steady_clock::now();
        std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
        auto duration = std::chrono::duration_cast< TimeT>
                            (std::chrono::steady_clock::now() - start);
        return duration.count();
    }
};

using measureMicroSecs = measure<std::chrono::microseconds>;

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {

namespace DataHandling {

DECLARE_ALGORITHM(LoadDNSEvent)

const std::string LoadDNSEvent::INSTRUMENT_NAME = "DNS";

void LoadDNSEvent::init() {
  /// Initialise the properties

  const std::vector<std::string> exts{".mdat"};
  declareProperty(Kernel::make_unique<FileProperty>("InputFile", "",
                                                    FileProperty::Load, exts),
                  "The XML or Map file with full path.");

  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "ChopperChannel", 1, boost::shared_ptr<BoundedValidator<uint>>(new BoundedValidator<uint>(0, 4)), Kernel::Direction::Input),
                  "The Chopper Channel");

  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "MonitorChannel", 1, boost::shared_ptr<BoundedValidator<uint>>(new BoundedValidator<uint>(0, 4)), Kernel::Direction::Input),
                  "The Monitor Channel");

  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "ChopperPeriod", EMPTY_INT(), Kernel::Direction::Input),
                  "The Chopper Period");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "MaxDeltaT", EMPTY_INT(), Kernel::Direction::Input),
                  "The Max Delta T");

  declareProperty(
      make_unique<WorkspaceProperty<DataObjects::EventWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "The name of the output workspace.");
}


/// Run the algorithm
void LoadDNSEvent::exec() {
  const size_t DUMMY_SIZE = 42;
  EventWorkspace_sptr outputWS =  boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 960*128, DUMMY_SIZE, DUMMY_SIZE));

  runLoadInstrument(INSTRUMENT_NAME, outputWS);
  outputWS->instrumentParameters().getType<uint>("chopper", "channel");

  g_log.notice() << "ChopperChannel: " << static_cast<uint>(getProperty("ChopperChannel")) << std::endl;
  g_log.notice() << "MonitorChannel: " << static_cast<uint>(getProperty("MonitorChannel")) << std::endl;
  chopperChannel = static_cast<uint>(getProperty("ChopperChannel"));
  monitorChannel = static_cast<uint>(getProperty("MonitorChannel"));
  const auto chopperChannels = outputWS->instrumentParameters().getType<uint>("chopper", "channel");
  const auto monitorChannels = outputWS->instrumentParameters().getType<uint>("monitor", "channel");
  chopperChannel = chopperChannel != 0 ? chopperChannel : chopperChannels.empty() ? 1 : chopperChannels.at(0);
  monitorChannel = monitorChannel != 0 ? monitorChannel : monitorChannels.empty() ? 1 : monitorChannels.at(0);

  chopperPeriod = static_cast<uint>(getProperty("ChopperPeriod"));


  // The number of steps depends on the type of input file
  // Set them to zero for the moment
  Progress progress(this, 0.0, 1.0, 4);

  try {
    ByteStream file(static_cast<std::string>(getProperty("InputFile")), endian::big, [this]() -> std::ostream& { return g_log.notice(); });
    //file.exceptions(std::ifstream::eofbit);

    auto elaapsedTimeParsing    = measureMicroSecs::execution([&](){ parse_File(file); });
    auto elaapsedTimeProcessing = measureMicroSecs::execution([&](){ populate_EventWorkspace(outputWS); });

    progress.doReport();

    g_log.notice()
        << "elaapsedTime Parsing\t= " << elaapsedTimeParsing
        << "\nelaapsedTime Processing\t= " << elaapsedTimeProcessing
        << "\nelaapsedTime Total  \t= " << elaapsedTimeParsing + elaapsedTimeProcessing
        << std::endl;


    progress.doReport();
  } catch (std::runtime_error e) {
    g_log.error(e.what());
  }
  //        g_log.notice()
  //            << std::setprecision(15) << std::fixed << "T"
  //            << ", " << double(event.timestamp) / 10000.0
  //            << std::endl;

  setProperty("OutputWorkspace", outputWS);
}

void LoadDNSEvent::populate_EventWorkspace(EventWorkspace_sptr eventWS) {
  std::sort(_eventAccumulator.neutronEvents.begin(), _eventAccumulator.neutronEvents.end(), [](auto l, auto r){ return l.timestamp < r.timestamp; });

  int64_t chopperTimestamp = 0;
  for (const auto &event : _eventAccumulator.neutronEvents) {
  //for (auto iter = _eventAccumulator.events.crbegin(); iter != _eventAccumulator.events.crend(); iter++) {
  //  const auto &event = *iter;

    //if ((event.timestamp - chopperTimestamp) > chopperPeriod) {
    if (event.position == 0xFFFF && event.channel == 0xFFFF) {
      chopperTimestamp = event.timestamp;
    } else /*if (event.timestamp - chopperTimestamp != 0)*/ {
      Mantid::DataObjects::EventList &eventList = eventWS->getSpectrum(0*event.channel * 960 + event.position);
      eventList.switchTo(TOF);
      eventList.addEventQuickly(Types::Event::TofEvent(double(event.timestamp - chopperTimestamp)));
    }
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

template<typename Iterable>
constexpr std::map<
   std::remove_reference_t<decltype(*std::declval<decltype(std::declval<Iterable>().cbegin())>())>,
  size_t
>
buildSkipTable(const Iterable &iterable) {
  std::map<
     std::remove_reference_t<decltype(*std::declval<decltype(std::declval<Iterable>().cbegin())>())>,
    size_t
  > skipTable;

  size_t i = iterable.size() - 1;
  for (auto c : iterable) {
    skipTable[c] = i;
    i--;
  }
  return skipTable;
}
}

std::vector<uint8_t> LoadDNSEvent::parse_Header(ByteStream &file) {
  // using Boyer-Moore String Search:
  LOG_INFORMATION("parse_Header...\n")
  static constexpr std::array<uint8_t, 8> header_sep {0x00, 0x00, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF};
  static const auto skipTable = buildSkipTable(header_sep);

  // search for header_sep and store actual header:
  std::vector<uint8_t> header;

  std::array<uint8_t, header_sep.size()> current_window;
  file.readRaw(current_window);

  int j = 0;
  while (!file.eof() && (j++ < 1024*8)) {
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
  //throw std::runtime_error(std::string("STOP!!!! please. (ugh!)"));
  return header;
}

void LoadDNSEvent::parse_File(ByteStream &file) {
  // File := Header Body
  std::vector<uint8_t> header = parse_Header(file);
  parse_BlockList(file, _eventAccumulator);
  parse_EndSignature(file);
}

void LoadDNSEvent::parse_BlockList(ByteStream &file, EventAccumulator &eventAccumulator) {
  LOG_INFORMATION("parse_BlockList...\n");
  // BlockList := DataBuffer BlockListTrail
  while (file.peek() != 0xFF) {
    parse_Block(file, eventAccumulator);
  }
}

void LoadDNSEvent::parse_Block(ByteStream &file, EventAccumulator &eventAccumulator) {
  LOG_INFORMATION("parse_Block...\n")
  // Block := DataBufferHeader DataBuffer
  parse_DataBuffer(file, eventAccumulator);
  parse_BlockSeparator(file);
}

void LoadDNSEvent::parse_BlockSeparator(ByteStream &file) {
  LOG_INFORMATION("parse_BlockSeparator...\n")
  const separator_t block_sep = (0x0000FFFF5555AAAA); // 0xAAAA5555FFFF0000; //
  auto separator = file.read(separator_t());
  if (separator != block_sep) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator)
                             + std::string("expected 0x") + n2hexstr(block_sep));
  }
}

void LoadDNSEvent::parse_DataBuffer(ByteStream &file, EventAccumulator &eventAccumulator) {
  LOG_INFORMATION("parse_DataBuffer...\n");
  const auto bufferHeader = parse_DataBufferHeader(file);

  const uint16_t dataLength = uint16_t(bufferHeader.bufferLength - 21);
  const auto event_count = dataLength / 3;
  LOG_INFORMATION("event_count = " << event_count << "\n");

  LOG_INFORMATION("parse_EventList...\n");
  for (uint16_t i = 0; i < event_count; i++) {
    parse_andAddEvent(file, bufferHeader, eventAccumulator);
  }

}

LoadDNSEvent::BufferHeader LoadDNSEvent::parse_DataBufferHeader(ByteStream &file) {
  LOG_INFORMATION("parse_DataBufferHeader...\n")
  BufferHeader header = {};
  file.read<2>(header.bufferLength);
  file.extractDataChunk<2>()
        .readBits<1>(header.bufferType)
        .readBits<15>(header.bufferVersion);
  file.read<2>(header.headerLength);
  file.read<2>(header.bufferNumber);
  file.read<2>(header.runId);
  file.read<1>(header.mcpdId);
  file.extractDataChunk<1>()
        .skipBits<6>()
        .readBits<2>(header.deviceStatus);
  file.read<6>(header.timestamp);
  file.skip<24>();
  return header;

}


void LoadDNSEvent::parse_EndSignature(ByteStream &file) {
  LOG_INFORMATION("parse_EndSignature...\n")
  const separator_t closing_sig = (0xFFFFAAAA55550000); // 0x00005555AAAAFFFF; //
  auto separator = file.read(separator_t());
  if (separator != closing_sig) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator)
                             + std::string("expected 0x") + n2hexstr(closing_sig));
  }
}

}// namespace Mantid
}// namespace DataHandling


