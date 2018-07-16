#include <sstream>

#include "MantidDataHandling/LoadDNSEvent.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"

#include <ios>
#include <fstream>
#include <vector>
#include <map>
#include <stdexcept>


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
      make_unique<WorkspaceProperty<DataObjects::GroupingWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "The name of the output workspace.");
}

/// Run the algorithm
void LoadDNSEvent::exec() {
  // The number of steps depends on the type of input file
  // Set them to zero for the moment
  Progress progress(this, 0.0, 1.0, 0);
  try {
    byte_stream file(static_cast<std::string>(getProperty("InputFile")), std::ios_base::binary);
    file.exceptions(std::ifstream::eofbit);
    parse_File(file);
  } catch (std::runtime_error e) {
    g_log.error(e.what());
  }
}

std::vector<uint8_t> LoadDNSEvent::parse_Header(byte_stream &file) {
  // using Boyer-Moore String Search:
  g_log.debug() << "parse_Header...\n";
  const std::array<uint8_t, 8> header_sep {0x00, 0x00, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF};

  std::map<uint8_t, size_t> skipTable;
  auto i = header_sep.size() - 1;
  for (auto c : header_sep) {
    skipTable[c] = i;
    i--;
  }

  // search for header_sep and store actual header:
  std::vector<uint8_t> header;

  std::array<uint8_t, header_sep.size()> current_window;
  file.read(current_window);

  int j = 0;
  while (!file.eof() && j++ < 1024*16) {
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
      file.read(*(win_data + (header_sep.size() - skip_length)), skip_length);
    }
  }
  return header;
}

LoadDNSEvent::event_list_t LoadDNSEvent::parse_File(byte_stream &file) {
  // File := Header Body
  auto header = parse_Header(file);
  g_log.debug() << std::string((char*)header.data(), header.size()) << "\n";

  //parse_BlockList(file);
  g_log.debug() << "parse_BlockList...\n";
  // BlockList := DataBuffer BlockListTrail
  event_list_t event_list;
  while (file.peek() != 0xFF) {
    //parse_Block(file, event_list);
    g_log.debug() << "parse_Block...\n";
    // Block := DataBufferHeader DataBuffer
    //parse_DataBuffer(file, event_list);  g_log.debug() << "parse_DataBuffer...\n";
    //const auto header = parse_DataBufferHeader(file);
    g_log.debug() << "parse_DataBufferHeader...\n";
    const auto header = file.read(buffer_header_t());
    g_log.debug() << "buffer length: " << convert_endianness(header[0]) << "\n";

    const auto event_count = (convert_endianness(header[0]) - 21) / 3;
    //parse_EventList(file, event_list, event_count);
    g_log.debug() << "parse_EventList...\n";
    for (uint16_t i = 0; i < event_count; i++) {
      //event_list.push_back(parse_Event(file));
      g_log.debug() << "parse_Event...\n" ;
      event_t event;
      file.read(event, sizeof(event));
      event_list.push_back(event);
    }
    //parse_BlockSeparator(file);
    g_log.debug() << "parse_BlockSeparator...\n";
    const separator_t block_sep = convert_endianness(0x0000FFFF5555AAAA); // 0xAAAA5555FFFF0000; //
    auto separator = file.read(separator_t());
    if (separator != block_sep) {
      throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator));
    }
  }
  g_log.notice() << "" << event_list.size() << " events found"<< "\n";
//  for (const auto event : event_list) {
//    g_log.notice() << "0x" << n2hexstr(event) << "\n";
//  }
  //parse_EndSignature(file);
  g_log.debug() << "parse_EndSignature...\n" ;
  const separator_t closing_sig = convert_endianness(0xFFFFAAAA55550000); // 0x00005555AAAAFFFF; //
  auto separator = file.read(separator_t());
  if (separator != closing_sig) {
    throw std::runtime_error(std::string("File Integrety LOST. (ugh!) 0x") + n2hexstr(separator));
  }
  return event_list;
}

} // namespace Mantid
} // namespace DataHandling
