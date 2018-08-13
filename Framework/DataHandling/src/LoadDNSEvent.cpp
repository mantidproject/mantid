#include <sstream>

#include "MantidDataHandling/LoadDNSEvent.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/NumericAxis.h"
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

void LoadDNSEvent::init() {
  /// Initialise the properties

  const std::vector<std::string> exts{".mdat"};
  declareProperty(Kernel::make_unique<FileProperty>("InputFile", "",
                                                    FileProperty::Load, exts),
                  "The XML or Map file with full path.");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "ChopperPeriod", EMPTY_INT(), Kernel::Direction::Input),
                  "The Chopper Period");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "MaxDeltaT", EMPTY_INT(), Kernel::Direction::Input),
                  "The Max Delta T");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<uint>>(
                      "MaxCount", EMPTY_INT(), Kernel::Direction::Input),
                  "The Max Count");

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
    ByteStream file(static_cast<std::string>(getProperty("InputFile")), endian::big, [this]() -> std::ostream& { return g_log.notice(); });
    //file.exceptions(std::ifstream::eofbit);

    EventAccumulator eventAccumulator(g_log, progress, static_cast<uint>(getProperty("ChopperPeriod")));
    eventAccumulator.chopperFinder.setChopperCandidate(1);
    auto elaapsedTimeParsing    = measureMicroSecs::execution([&](){ parse_File(file, eventAccumulator); });
    auto elaapsedTimeProcessing = measureMicroSecs::execution([&](){ eventAccumulator.postProcessData(outputWS); });

    const uint maxN = static_cast<uint>(getProperty("MaxCount"));

    populate_EventWorkspace(outputWS, eventAccumulator);

    progress.doReport();

    g_log.notice()
        << "elaapsedTime Parsing\t= " << elaapsedTimeParsing
        << "\nelaapsedTime Processing\t= " << elaapsedTimeProcessing
        << "\nelaapsedTime Total  \t= " << elaapsedTimeParsing + elaapsedTimeProcessing
        << std::endl;




    std::ofstream textfile(static_cast<std::string>(getProperty("InputFile")) + ".txt");

    std::set<int64_t> allDeltaTs;
    for (const auto &pair1 : eventAccumulator.chopperFinder.dataSourceInfos) {
      for (const auto &pair2 : pair1.second.deltaTFrequencies ) {
        allDeltaTs.insert(pair2.first);
      }
    }
    textfile << "\t";
    int i = 0;
    for (const auto &deltaT : allDeltaTs ) {
      i++;
      if (i > maxN) {
        break;
      }
      textfile << deltaT << "\t";
    }
    textfile << "\n";

    for (const auto &pair1 : eventAccumulator.chopperFinder.dataSourceInfos) {
      logTuple(textfile, pair1.first.mcpdId, pair1.first.dataId, pair1.first.trigId) << "\t";
      i = 0;

      for (const int64_t &deltaT : allDeltaTs ) {
        i++;
        if (i > maxN) {
          break;
        }

        if (pair1.second.deltaTFrequencies.find(deltaT) != pair1.second.deltaTFrequencies.end()) {
          textfile << pair1.second.deltaTFrequencies.at(deltaT) << "\t";
        } else {
          textfile << "0\t";
        }
      }
      textfile << "\n";
    }



    std::stringstream sbx;
    int k = 0;
    for (const auto &event : eventAccumulator.events) {

      if (k++ > maxN) { break; }

      sbx << event.timestamp << "\n";

    }
    textfile << sbx.str();

    textfile.close();


    {
      g_log.notice() << " Events found (" << eventAccumulator.eventsOut.size() << " of " << eventAccumulator.events.size() << "):" << "\n";

      for (const auto &pair : eventAccumulator.chopperFinder.dataSourceInfos) {
        logTuple(g_log.notice(), pair.first.mcpdId, pair.first.dataId, pair.first.trigId) << "\t";
      }
      g_log.notice() << "\n";

      int maxK = static_cast<uint>(getProperty("MaxCount"));
      std::stringstream sb;
      for (const auto &events : eventAccumulator.eventsOut) {
        maxK--;
        if (maxK <= 0) {
          break;
        }
        for (const auto &pair : eventAccumulator.chopperFinder.dataSourceInfos) {
            const auto searchResult = events.find(pair.first);
            if (searchResult != events.end()) {
              sb << searchResult->second.timestamp;
            }
            sb << "\t";
        }
        sb << std::endl;
      }
      g_log.notice() << sb.str();
    }

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

//template<class EventWorkspace_sptr>
void LoadDNSEvent::populate_EventWorkspace(EventWorkspace_sptr eventWS, LoadDNSEvent::EventAccumulator eventAccumulator) {
  return;
  std::map<uint64_t, uint32_t> histogram;

  //eventWS->getSpectrum(1).setDetectorID();

  for (auto event : eventAccumulator.tmpEvents) {
    //const uint16_t channel = event.mcpdId << 8 | event.data.neutron.modId << 5 | event.data.neutron.slotId;
    //const uint16_t position = event.data.neutron.position;
    const double tof = double(event.tof);//event.timestamp);

    //eventList.addDetectorID();
    EventList &eventList = eventWS->getSpectrum(0*event.channel * 960 + event.position);
    //eventList.switchTo(TOF);
    eventList.addEventQuickly(Types::Event::TofEvent(tof));
  }

  for (auto pair : histogram) {
    g_log.notice() << pair.first << "\t" << pair.second << std::endl;
  }
  return;


  //eventList.setHistogram()
  //el.setHistogram(this->m_xbins);
  //eventList.setBinEdges(); ???

  const auto &xVals = eventWS->x(0);
  const size_t xSize = xVals.size();
  auto ax0 = new NumericAxis(xSize);
  std::cout << "xSize = " << xSize << std::endl;
  // X-axis is 1 <= wavelength <= 6 Angstrom with step of 0.05
  ax0->setUnit("Wavelength");
  for (size_t i = 0; i < xSize; i++) {
    ax0->setValue(i, 1.0 + 0.05 * xVals[i]);
  }
  eventWS->replaceAxis(0, ax0);

  for (Event event : eventAccumulator.events) {
    if (event.eventId == event_id_e::NEUTRON){
      const uint16_t position = event.data.neutron.position;
      const double tof = double(event.timestamp) / 10000.0;

      //eventList.addDetectorID();
      EventList &eventList = eventWS->getSpectrum(position);
      eventList.switchTo(TOF);
      eventList.addEventQuickly(Types::Event::TofEvent(tof));
    }
  }


}

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

/*
LoadDNSEvent::eventAccumulator_t LoadDNSEvent::parse_FileX(ByteStream &file) {
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


void LoadDNSEvent::parse_File(ByteStream &file, EventAccumulator &eventAccumulator) {
  // File := Header Body
  std::vector<uint8_t> header = parse_Header(file);

  parse_BlockList(file, eventAccumulator);
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
  LOG_INFORMATION("parse_DataBuffer...\n")
  const auto header = parse_DataBufferHeader(file);
  eventAccumulator.registerDataBuffer(header);
  LOG_INFORMATION("buffer length: " << header.bufferLength << "\n");
  parse_EventList(file, eventAccumulator, uint16_t(header.bufferLength - 21));
}

LoadDNSEvent::BufferHeader LoadDNSEvent::parse_DataBufferHeader(ByteStream &file) {
  LOG_INFORMATION("parse_DataBufferHeader...\n")
  BufferHeader header = {};
  file.read<2>(header.bufferLength);
//  file.skip<2>();
file.extractDataChunk<2>()
        .readBits<1>(header.bufferType)
        .readBits<15>(header.bufferVersion);
  file.read<2>(header.headerLength);
//  file.skip<4>();
file.read<2>(header.bufferNumber);
file.read<2>(header.runId);
  file.read<1>(header.mcpdId);
//  file.skip<1>();
file.extractDataChunk<1>()
        .skipBits<6>()
        .readBits<2>(header.deviceStatus);
  file.read<6>(header.timestamp);
  file.skip<24>();
  return header;

}

void LoadDNSEvent::parse_EventList(ByteStream &file, EventAccumulator &eventAccumulator, const uint16_t &dataLength) {
  LOG_INFORMATION("parse_EventList...\n")
  const auto event_count = dataLength / 3;
  LOG_INFORMATION("event_count = " << event_count << "\n")
  for (uint16_t i = 0; i < event_count; i++) {
    Event event = {};
    const auto dataChunk = file.extractDataChunk<6>().readBits<1>(event.eventId);

    switch (event.eventId) {
      case event_id_e::TRIGGER: {
        TriggerEventData eventData = {};
        dataChunk
            //.skip<28>()
            .readBits<3>(eventData.trigId)
            .readBits<4>(eventData.dataId)
            .readBits<21>(eventData.data)
            .readBits<19>(event.timestamp);
        event.data.trigger = eventData;
        eventAccumulator.addTriggerEvent(event);
      } break;
      case event_id_e::NEUTRON: {
        NeutronEventData eventData = {};
        dataChunk
            //.skip<28>()
            .readBits<3>(eventData.modId)
            .skipBits<1>()
            .readBits<4>(eventData.slotId)
            .readBits<10>(eventData.amplitude)
            .readBits<10>(eventData.position)
            .readBits<19>(event.timestamp);
        event.data.neutron = eventData;
        eventAccumulator.addNeutronEvent(event);
      } break;
    default:
      // Panic!!!!
      g_log.error() << "unknow event id 0x" << n2hexstr(event.eventId) << "\n";
      break;
    }
  }
}

LoadDNSEvent::Event LoadDNSEvent::parse_Event(ByteStream &file, const HeaderId &headerId) {

  Event event = {};
  file.extractDataChunk<6>()
      //.readBits<1>(event.eventId)
      .skipBits<29>()
      .readBits<19>(event.timestamp);
  return event;


//  const auto dataChunk = file.extractDataChunk<6>().readBits<1>(event.eventId);
//  switch (event.eventId) {
//    case event_id_e::TRIGGER: {
//      TriggerEventData eventData = {};
//      dataChunk
//          //.skip<28>()
//          .readBits<3>(eventData.trigId)
//          .readBits<4>(eventData.dataId)
//          .readBits<21>(eventData.data)
//          .readBits<19>(event.timestamp);
//      event.data.trigger = eventData;
//    } break;
//    case event_id_e::NEUTRON: {
//      NeutronEventData eventData = {};
//      dataChunk
//          //.skip<28>()
//          .readBits<3>(eventData.modId)
//          .skipBits<1>()
//          .readBits<4>(eventData.slotId)
//          .readBits<10>(eventData.amplitude)
//          .readBits<10>(eventData.position)
//          .readBits<19>(event.timestamp);
//      event.data.neutron = eventData;
//    } break;
//  default:
//    // Panic!!!!
//    g_log.error() << "unknow event id 0x" << n2hexstr(event.eventId) << "\n";
//    break;
//  }

//  event.headerId = headerId;
//  return event;
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


