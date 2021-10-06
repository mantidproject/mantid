#ifndef MANTID_DATAHANDLING_LoadDNSEvent_H_
#define MANTID_DATAHANDLING_LoadDNSEvent_H_

#include "BitStream.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/System.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <array>
#include <fstream>
#include <limits>
//#include <iterator>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::DataHandling {

/**
  LoadDNSEvent

  Algorithm used to generate an EventWorkspace from a DNS PSD listmode (.mdat)
  file.

  @author Joachim Coenen, Jülich Centre for Neutron Science
  @date 2018-08-16

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport LoadDNSEvent : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  ///
  const std::string name() const override { return "LoadDNSEvent"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from the DNS PSD detector to a Mantid EventWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  static const std::string INSTRUMENT_NAME;
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  struct BufferHeader {
    uint16_t bufferLength;
    uint16_t bufferVersion;
    uint16_t headerLength;
    uint16_t bufferNumber;
    uint16_t runId;
    uint8_t mcpdId;
    uint8_t deviceStatus;
    uint64_t timestamp;
  };

public:
  enum event_id_e { NEUTRON = 0, TRIGGER = 1 };

  struct CompactEvent {
    uint64_t timestamp;
  };

private:
  struct EventAccumulator {
    //! Neutron Events for each pixel
    std::vector<std::vector<CompactEvent>> neutronEvents;
    std::vector<CompactEvent> triggerEvents;
  };

  uint32_t chopperChannel;
  // uint32_t monitorChannel;
  bool discardPreChopperEvents;
  bool setBinBoundary;

  void runLoadInstrument(std::string instrumentName, DataObjects::EventWorkspace_sptr &eventWS);

  void populate_EventWorkspace(Mantid::DataObjects::EventWorkspace_sptr eventWS,
                               EventAccumulator &finalEventAccumulator);

  EventAccumulator parse_File(FileByteStream &file, const std::string fileName);
  std::vector<uint8_t> parse_Header(FileByteStream &file);

  std::vector<std::vector<uint8_t>> split_File(FileByteStream &file, const unsigned maxChunckCount);

  void parse_BlockList(VectorByteStream &file, EventAccumulator &eventAccumulator);
  void parse_Block(VectorByteStream &file, EventAccumulator &eventAccumulator);
  void parse_BlockSeparator(VectorByteStream &file);
  void parse_DataBuffer(VectorByteStream &file, EventAccumulator &eventAccumulator);
  BufferHeader parse_DataBufferHeader(VectorByteStream &file);

  inline size_t getWsIndex(const uint8_t &channel, const uint16_t &position) {
    const uint32_t channelIndex = ((channel & 0b11100000u) >> 2u) | (channel & 0b00000111u);
    return channelIndex * 1024 + position;
  }

  inline void parse_andAddEvent(VectorByteStream &file, const BufferHeader &bufferHeader,
                                EventAccumulator &eventAccumulator);

  void parse_EndSignature(FileByteStream &file);
};

} // namespace Mantid::DataHandling

#endif /* MANTID_DATAHANDLING_LoadDNSEvent_H_ */
