// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_DATAHANDLING_LOADNGEM_H_
#define MANTID_DATAHANDLING_LOADNGEM_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace DataHandling {

static constexpr uint64_t CONTIN_ID_VALUE = 0x4F;

/// Generic event to separate bits.
struct GenericEvent {
  uint64_t t0id : 24;      // T0 ID
  uint64_t reserved2 : 32; // Reserved for non-generics
  uint64_t contin : 8;     // 0x4F Continuation Code
  uint64_t reserved1 : 56; // Reserved for non-generics
  uint64_t id : 8;         // 0x4E Event ID
};

/// Indicate time 0, the start of a new frame.
struct T0FrameEvent {
  uint64_t t0id : 24;       // T0 ID
  uint64_t eventCount : 32; // Event Count
  uint64_t contin : 8;      // 0x4F Continuation Code
  uint64_t totalLoss : 24;  // Total loss count
  uint64_t eventLoss : 20;  // Event loss count
  uint64_t frameLoss : 12;  // Frame loss count
  uint64_t id : 8;          // 0x4E Event ID
  static constexpr int T0_IDENTIFIER = 0x4E;
  bool check() const {
    return id == T0_IDENTIFIER && contin == CONTIN_ID_VALUE;
  }
};

/// A detected neutron.
struct CoincidenceEvent {
  uint64_t t0id : 24;         // T0 ID
  uint64_t clusterTimeY : 10; // Integrated time of the cluster on the Y side
                              // (5ns pixel)
  uint64_t timeDiffY : 6; // Time lag from first to last detection on Y (5ns)
  uint64_t clusterTimeX : 10; // Integrated time of the cluster on the X side
                              // (5ns pixel)
  uint64_t timeDiffX : 6; // Time lag from first to last detection on X (5ns)
  uint64_t contin : 8;    // 0x4F Continuation Code
  uint64_t lastY : 7;     // Y position of pixel detected last
  uint64_t firstY : 7;    // Y position of pixel detected first
  uint64_t lastX : 7;     // X position of pixel detected last
  uint64_t firstX : 7;    // X position of pixel detected first
  uint64_t timeOfFlight : 28; // Difference between T0 and detection (1ns)
  uint64_t id : 8;            // 0x47 Event ID.

  uint64_t avgX() const { return (firstX + lastX) / 2; }
  uint64_t avgY() const { return (firstY + lastY) / 2; }
  static constexpr int COINCIDENCE_IDENTIFIER = 0x47;
  bool check() {
    return id == COINCIDENCE_IDENTIFIER && contin == CONTIN_ID_VALUE;
  }
  uint64_t getPixel() const {
    return avgX() + (avgY() << 7); // Increase Y significance by 7 bits to
                                   // account for 128x128 grid.
  }
};

/// Holds the 128 bit words from the detector.
struct DetectorWord {
  uint64_t words[2]; // Array holding the word from the detector split in two.
};

/// Is able to hold all versions of the data words in the same memory location.
union EventUnion {
  GenericEvent generic;
  T0FrameEvent tZero;
  CoincidenceEvent coincidence;
  DetectorWord splitWord;
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
  const std::string category() const override { return "DataHandling\\NGEM"; };
  /// Should the loader load multiple files into one workspace.
  bool loadMutipleAsOne() override { return true; }

  /// The confidence that an algorithm is able to load the file.
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialise the algorithm.
  void init() override;
  /// Execute the algorithm.
  void exec() override;
  /// Load a file into the event lists.
  void loadSingleFile(const std::vector<std::string> &filePath,
                      int &eventCountInFrame, int &maxToF, int &minToF,
                      int &rawFrames, int &goodFrames, const int &minEventsReq,
                      const int &maxEventsReq, MantidVec &frameEventCounts,
                      std::vector<DataObjects::EventList> &events,
                      std::vector<DataObjects::EventList> &eventsInFrame,
                      const size_t &totalFilePaths, int &fileCount);
  /// Add some text information to the sample logs.
  void addToSampleLog(const std::string &logName, const std::string &logText,
                      DataObjects::EventWorkspace_sptr &ws);
  /// Add some number information to the sample logs.
  void addToSampleLog(const std::string &logName, const int &logNumber,
                      DataObjects::EventWorkspace_sptr &ws);
  /// Check that a file to be loaded is in 128 bit words.
  size_t verifyFileSize(std::ifstream &file);
  /// Reports progress and checks cancel flag.
  bool reportProgressAndCheckCancel(size_t &numProcessedEvents,
                                    int &eventCountInFrame,
                                    const size_t &totalNumEvents,
                                    const size_t &totalFilePaths,
                                    const int &fileCount);
  /// Create a workspace to store the number of counts per frame.
  void createCountWorkspace(const std::vector<double> &frameEventCounts);
  /// Load the instrument and attach to the data workspace.
  void loadInstrument(DataObjects::EventWorkspace_sptr &dataWorkspace);
  /// Validate the imputs into the algorithm, overrides.
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADNGEM_H_
