// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BitStream.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/FileDescriptor.h"

#include "MantidTypes/Core/DateAndTime.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::DataHandling {

/**
  LoadDNSEvent

  Algorithm used to generate an EventWorkspace from a DNS PSD listmode (.mdat)
  file.

  @author Joachim Coenen, Thomas Mueller, Jülich Centre for Neutron Science
  @date 2022-01-10

*/

class MANTID_DATAHANDLING_DLL LoadDNSEvent : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  ///
  const std::string name() const override { return "LoadDNSEvent"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from the DNS PSD detector to a Mantid EventWorkspace.";
  }

  // Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {}; }
  // Algorithm's category for identification
  const std::string category() const override { return "DataHandling"; }
  // Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  // Initialise the properties
  void init() override;
  // Run the algorithm
  void exec() override;

  struct BufferHeader {
    uint16_t bufferLength = 0;
    uint16_t bufferVersion = 0;
    uint16_t headerLength = 0;
    uint16_t bufferNumber = 0;
    uint16_t runId = 0;
    uint8_t mcpdId = 0;
    uint8_t deviceStatus = 0;
    uint64_t timestamp = 0;
  };

public:
  enum event_id_e { NEUTRON = 0, TRIGGER = 1 };

  struct CompactEvent {
    uint64_t timestamp = 0;
  };

private:
  struct EventAccumulator {
    // Neutron Events for each pixel
    std::vector<std::vector<CompactEvent>> neutronEvents;
    std::vector<CompactEvent> triggerEvents;
  };

  struct TriggerEvent {
    bool isChopperTrigger = false;
    CompactEvent event = {};
  };

  struct NeutronEvent {
    size_t wsIndex = 0;
    CompactEvent event = {};
  };

  uint32_t m_chopperChannel = 2;
  uint32_t m_detectorPixelCount = 0;

  bool m_discardPreChopperEvents = true;
  bool m_setBinBoundary = false;

  void populate_EventWorkspace(Mantid::DataObjects::EventWorkspace_sptr &eventWS,
                               EventAccumulator &finalEventAccumulator);

  EventAccumulator parse_File(FileByteStream &file, const std::string &fileName);
  std::vector<uint8_t> parse_Header(FileByteStream &file);

  std::vector<std::vector<uint8_t>> split_File(FileByteStream &file, const unsigned maxChunckCount);

  void parse_BlockList(VectorByteStream &file, EventAccumulator &eventAccumulator);
  void parse_Block(VectorByteStream &file, EventAccumulator &eventAccumulator);
  void parse_BlockSeparator(VectorByteStream &file);
  void parse_DataBuffer(VectorByteStream &file, EventAccumulator &eventAccumulator);
  BufferHeader parse_DataBufferHeader(VectorByteStream &file);

  inline void parse_andAddEvent(VectorByteStream &file, const BufferHeader &bufferHeader,
                                EventAccumulator &eventAccumulator);

  void parse_EndSignature(FileByteStream &file);

  TriggerEvent processTrigger(const uint64_t &data, const LoadDNSEvent::BufferHeader &bufferHeader);
  NeutronEvent processNeutron(const uint64_t &data, const LoadDNSEvent::BufferHeader &bufferHeader);
};

} // namespace Mantid::DataHandling
