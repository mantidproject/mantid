// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_DATAHANDLING_LOADNGEM_H_
#define MANTID_DATAHANDLING_LOADNGEM_H_

#include "MantidAPI/IFileLoader.h"

namespace Mantid {
namespace DataHandling {

#define CID_VALUE 0x4F;

// Indicate time 0, the start of a new frame.
struct T0FrameEvent {
  uint8_t ID : 8;           // 0x4E Event ID
  uint16_t frameLoss : 12;  // Frame loss count
  uint32_t eventLoss : 20;  // Event loss count
  uint8_t CID : 8;          // 0x4F Continuation Code
  uint32_t eventCount : 32; // Event Count
  uint32_t TOID : 24;       // T0 ID;
};

struct CoincidenceEvent {
  uint8_t ID : 8;             // 0x47 Event ID.
  uint32_t timeOfFlight : 28; // Difference between T0 and detection (1ns)
  uint8_t firstX : 7;         // X position of pixel detected first
  uint8_t lastX : 7;          // X position of pixel detected last
  uint8_t firstY : 7;         // Y position of pixel detected first
  uint8_t lastY : 7;          // Y position of pixel detected last
  uint8_t CID : 8;            // 0x4F Continuation Code
  uint8_t timeDiffX : 6; // Time lag from first to last detection on X (5ns)
  uint16_t clusterTimeX : 10; // Integrated time of the cluster on the X side
                              // (5ns pixel)
  uint8_t timeDiffY : 7; // Time lag from first to last detection on Y (5ns)
  uint16_t clusterTimeX : 10; // Integrated time of the cluster on the Y side
                              // (5ns pixel)
};

class DLLExport LoadNGEM : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification.
  const std::string name() const override { return "LoadNGEM"; }
  /// The purpose of the algorithm.
  const std::string summary() const override {
    return "Load a file or range of files created by the nGEM detector into a "
           "workspace.";
  };
  /// Algorithm's Version for identification.
  int version() const override { return 1; }
  /// Algorithm's category for identification.
  const std::string category() const override { return "DataHandling\\nGEM"; };

  /// The confidence that an algorithm is able to load the file.
  int confidence(Kernel::FileDescriptor &descriptor) const override;
  /// Load multiple files into one workspace.
  bool loadMutipleAsOne() override;

private:
  /// Initialise the algorithm.
  void init() override;
  /// Execute the algorithm.
  void exec() override;
  /// Byte swap a word to be correct on x86 and x64 architecture.
  static uint64_t swap_uint64(uint64_t word);
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADNGEM_H_
