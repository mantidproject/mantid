#include <sstream>

#include "MantidDataHandling/LoadDNSEvent.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
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

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {

namespace DataHandling {

DECLARE_ALGORITHM(LoadDNSEvent)

void LoadDNSEvent::init() {
  /// Initialise the properties

  const std::vector<std::string> exts{".mdat"};
  declareProperty(Kernel::make_unique<FileProperty>("InputFile", "",
                                                    FileProperty::Load, exts),
                  "The XML or Map file with full path.");

  declareProperty(
      make_unique<WorkspaceProperty<DataObjects::EventWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "The name of the output workspace.");
}


/// Run the algorithm
void LoadDNSEvent::exec() {
  // The number of steps depends on the type of input file
  // Set them to zero for the moment
  Progress progress(this, 0.0, 1.0, 4);

  const size_t DUMMY_SIZE = 42;
  EventWorkspace_sptr outputWS =  boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 960*128, DUMMY_SIZE, DUMMY_SIZE));


  try {
    BitStream file(static_cast<std::string>(getProperty("InputFile")), endian::big, [this]() -> std::ostream& { return g_log.notice(); });
    //file.exceptions(std::ifstream::eofbit);

    EventAccumulator eventAccumulator(g_log, progress);
    auto elaapsedTime = measure<std::chrono::microseconds>::execution([&](){ parse_File(file, eventAccumulator); });
    eventAccumulator.postProcessData();

    progress.doReport();

    g_log.notice()
        << "elaapsedTime = " << elaapsedTime
        << std::endl;

//    g_log.notice() << " Events found:" << "\n";
//    size_t k = 0;
//    for (auto &event : eventAccumulator.events)/**/ {
//      k++;
//      if (k >= 2000) {
//        break;
//      }
//      g_log.notice()
//          << std::setprecision(15) << std::fixed << "N"
//          << ", " << double(event.timestamp) / 10000.0
//          << std::endl;
//    }


    progress.doReport();
  } catch (std::runtime_error e) {
    g_log.error(e.what());
  }

  setProperty("OutputWorkspace", outputWS);
}

//void LoadDNSEvent::populate_EventWorkspace(EventWorkspace_sptr eventWS, LoadDNSEvent::eventAccumulator_t eventList) {
//  EventList &eventListl = outputWS->getSpectrum(j);
//  eventListl.switchTo(TOF);
//}
namespace {
//std::result_of<decltype(&foo::bar)(foo)>::type
//decltype(*Iterable::cbegin())
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

std::vector<uint8_t> LoadDNSEvent::parse_Header(BitStream &file) {
  // using Boyer-Moore String Search:
  LOG_INFORMATION("parse_Header...\n")
  static constexpr std::array<uint8_t, 8> header_sep {0x00, 0x00, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF};
  static const auto skipTable = buildSkipTable(header_sep);

  // search for header_sep and store actual header:
  std::vector<uint8_t> header;

  std::array<uint8_t, header_sep.size()> current_window;
  g_log.notice() << "Hi!" << "\n";
  file.readRaw(current_window);

  int j = 0;
  while (!file.eof() && (j++ < 1024*1)) {
    if (current_window == header_sep) {
      return header;
    } else {
      auto iter = skipTable.find(*current_window.rbegin());
      size_t skip_length = (iter == skipTable.end()) ? header_sep.size() : iter->second;

      const auto orig_header_size = header.size();
      header.resize(header.size() + skip_length);
      const auto win_data = current_window.data();
      std::copy(win_data, win_data + skip_length, header.data() + orig_header_size);

      std::copy(win_data + skip_length, win_data + header_sep.size(), win_data);
      file.readRaw(current_window, skip_length * 8);
    }
  }
  //throw std::runtime_error(std::string("STOP!!!! please. (ugh!)"));
  return header;
}

/*
LoadDNSEvent::eventAccumulator_t LoadDNSEvent::parse_FileX(BitStream &file) {
  // File := Header Body
  auto header = parse_Header(file);
  g_log.information() << std::string((char*)header.data(), header.size()) << "\n";

  //parse_BlockList(file);
  LOG_INFORMATION("parse_BlockList...\n")
  // BlockList := DataBuffer BlockListTrail
  eventAccumulator_t eventAccumulator;
  while (file.peek() != 0xFF) {
    //parse_Block(file, eventAccumulator);
    LOG_INFORMATION("parse_Block...\n")
    // Block := DataBufferHeader DataBuffer
    //parse_DataBuffer(file, eventAccumulator);  LOG_INFORMATION("parse_DataBuffer...\n")
    //const auto header = parse_DataBufferHeader(file);
    LOG_INFORMATION("parse_DataBufferHeader...\n")
    const auto header = file.read(buffer_header_t());
    g_log.information() << "buffer length: " << convert_endianness(header[0]) << "\n";

    const auto event_count = (convert_endianness(header[0]) - 21) / 3;
    //parse_EventList(file, eventAccumulator, event_count);
    LOG_INFORMATION("parse_EventList...\n")
    for (uint16_t i = 0; i < event_count; i++) {
      //eventAccumulator.push_back(parse_Event(file));
      LOG_INFORMATION("parse_Event...\n" )
      event_t event = {};
      file.read(event, sizeof(event));
      eventAccumulator.push_back(event);
    }
    //parse_BlockSeparator(file);
    LOG_INFORMATION("parse_BlockSeparator...\n")
    const separator_t block_sep = convert_endianness(0x0000FFFF5555AAAA); // 0xAAAA5555FFFF0000; //
    auto separator = file.read(separator_t());
    if (separator != block_sep) {
      throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator));
    }
  }
  g_log.notice() << "" << eventAccumulator.size() << " events found"<< "\n";
//  for (const auto event : eventAccumulator) {
//    g_log.notice() << "0x" << n2hexstr(event) << "\n";
//  }
  //parse_EndSignature(file);
  LOG_INFORMATION("parse_EndSignature...\n" )
  const separator_t closing_sig = convert_endianness(0xFFFFAAAA55550000); // 0x00005555AAAAFFFF; //
  auto separator = file.read(separator_t());
  if (separator != closing_sig) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator));
  }
  return eventAccumulator;
}
*/


void LoadDNSEvent::parse_File(BitStream &file, EventAccumulator &eventAccumulator) {
  // File := Header Body
  std::vector<uint8_t> header = parse_Header(file);
//  g_log.notice() << std::string((char*)header.data(), header.size()) << "\n";
//  g_log.notice() <<"---header----- " << header.size() << "\n";
//  for (auto v : header) {
//    g_log.notice() << n2hexstr(v) << " ";
//  }
//  g_log.notice() <<"\n---header----- " << header.size() << "\n";

  parse_BlockList(file, eventAccumulator);
  parse_EndSignature(file);
}

void LoadDNSEvent::parse_BlockList(BitStream &file, EventAccumulator &eventAccumulator) {
  LOG_INFORMATION("parse_BlockList...\n")
  // BlockList := DataBuffer BlockListTrail
  while (file.peek() != 0xFF) {
    parse_Block(file, eventAccumulator);
  }
}

void LoadDNSEvent::parse_Block(BitStream &file, EventAccumulator &eventAccumulator) {
  LOG_INFORMATION("parse_Block...\n")
  // Block := DataBufferHeader DataBuffer
  parse_DataBuffer(file, eventAccumulator);
  parse_BlockSeparator(file);
}

void LoadDNSEvent::parse_BlockSeparator(BitStream &file) {
  const separator_t block_sep = (0x0000FFFF5555AAAA); // 0xAAAA5555FFFF0000; //
  auto separator = file.read(separator_t());
  if (separator != block_sep) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator)
                             + std::string("expected 0x") + n2hexstr(block_sep));
  }
}

void LoadDNSEvent::parse_DataBuffer(BitStream &file, EventAccumulator &eventAccumulator) {
  LOG_INFORMATION("parse_DataBuffer...\n")
  const auto header = parse_DataBufferHeader(file);
  auto headerId = eventAccumulator.registerDataBuffer(header);
  LOG_INFORMATION("buffer length: " << header.bufferLength << "\n")
  parse_EventList(file, eventAccumulator, headerId, uint16_t(header.bufferLength - header.headerLength));
}

LoadDNSEvent::BufferHeader LoadDNSEvent::parse_DataBufferHeader(BitStream &file) {
//  uint16_t bufferLength;
//  uint16_t bufferVersion;
//  BufferType bufferType;
//  uint16_t headerLength; //static const uint16_t headerLength = 21;
//  uint16_t bufferNumber;
//  uint16_t runId;
//  uint8_t  mcpdId;
//  DeviceStatus deviceStatus;
//  uint64_t timestamp;
  LOG_INFORMATION("parse_DataBufferHeader...\n")
  BufferHeader header = {};
  file
      .read<16>(header.bufferLength)
      .read<1>(header.bufferType)
      .read<15>(header.bufferVersion)
      .read<16>(header.headerLength)
      .read<16>(header.bufferNumber)
      .read<16>(header.runId)
      .read<8>(header.mcpdId)
      .skip<6>()
      .read<2>(header.deviceStatus)
      .read<48>(header.timestamp)
      .skip<192>();
  // g_log.notice() << n2binstr(header.timestamp) << std::endl;
  // g_log.notice() << n2binstr(header.bufferLength) << std::endl;
  return header;

}

void LoadDNSEvent::parse_EventList(BitStream &file, EventAccumulator &eventAccumulator, const HeaderId &headerId, const uint16_t &dataLength) {
  LOG_INFORMATION("parse_EventList...\n")
  const auto event_count = dataLength / 3;
  LOG_INFORMATION("event_count = " << event_count << "\n")
  for (uint16_t i = 0; i < event_count; i++) {
    eventAccumulator.addEvent(parse_Event(file, headerId));
    //
  }
}

LoadDNSEvent::Event LoadDNSEvent::parse_Event(BitStream &file, const HeaderId &headerId) {

  Event event = {};
  file.read<1>(event.eventId);
  uint8_t DUMMY;
  switch (event.eventId) {
    case event_id_e::TRIGGER: {
      TriggerEventData eventData = {};
      file
          //.skip<28>()
          .read<3>(eventData.trigId)
          .read<4>(eventData.dataId)
          .read<21>(eventData.data)
          .read<19>(event.timestamp);
      event.data.trigger = eventData;
    } break;
    case event_id_e::NEUTRON: {
      NeutronEventData eventData = {};
      file
          //.skip<28>()
          .read<3>(eventData.modId)
          .read<1>(DUMMY)
          .read<4>(eventData.slotId)
          .read<10>(eventData.amplitude)
          .read<10>(eventData.position)
          .read<19>(event.timestamp);
      event.data.neutron = eventData;
    } break;
  default:
    // Panic!!!!
    g_log.error() << "unknow event id 0x" << n2hexstr(event.eventId) << "\n";
    std::array<uint8_t, 6> dummy;
    file.readRaw<15+16+16>(dummy);
    break;
  }

  event.headerId = headerId;
  return event;
}

void LoadDNSEvent::parse_EndSignature(BitStream &file) {
  const separator_t closing_sig = (0xFFFFAAAA55550000); // 0x00005555AAAAFFFF; //
  auto separator = file.read(separator_t());
  if (separator != closing_sig) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator)
                             + std::string("expected 0x") + n2hexstr(closing_sig));
  }
}

}// namespace Mantid
}// namespace DataHandling




























