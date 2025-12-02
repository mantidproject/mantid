// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {

static constexpr uint64_t CONTIN_ID_VALUE = 0x4F;
static constexpr uint64_t EVENT_ID_MASK = 0x40;

/// Generic event to separate bits.
struct GenericEvent {
  uint64_t t0id : 24;      // T0 ID
  uint64_t reserved2 : 32; // Reserved for non-generics
  uint64_t contin : 8;     // 0x4F Continuation Code
  uint64_t reserved1 : 56; // Reserved for non-generics
  uint64_t id : 8;         // Event ID
  bool check() const {
    // as id is 8 bit, we can do a simple AND to check
    return (id & EVENT_ID_MASK) != 0 && contin == CONTIN_ID_VALUE;
  }
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
  bool check() const { return id == T0_IDENTIFIER && contin == CONTIN_ID_VALUE; }
};

/// A detected neutron.
struct CoincidenceEvent {
  uint64_t t0id : 24;         // T0 ID
  uint64_t clusterTimeY : 10; // Integrated time of the cluster on the Y side
                              // (5ns pixel)
  uint64_t timeDiffY : 6;     // Time lag from first to last detection on Y (5ns)
  uint64_t clusterTimeX : 10; // Integrated time of the cluster on the X side
                              // (5ns pixel)
  uint64_t timeDiffX : 6;     // Time lag from first to last detection on X (5ns)
  uint64_t contin : 8;        // 0x4F Continuation Code
  uint64_t lastY : 7;         // Y position of pixel detected last
  uint64_t firstY : 7;        // Y position of pixel detected first
  uint64_t lastX : 7;         // X position of pixel detected last
  uint64_t firstX : 7;        // X position of pixel detected first
  uint64_t timeOfFlight : 28; // Difference between T0 and detection (1ns)
  uint64_t id : 8;            // 0x47 Event ID.

  uint64_t avgX() const { return (firstX + lastX) / 2; }
  uint64_t avgY() const { return (firstY + lastY) / 2; }
  static constexpr int COINCIDENCE_IDENTIFIER = 0x47;
  bool check() { return id == COINCIDENCE_IDENTIFIER && contin == CONTIN_ID_VALUE; }
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

/// Holds variables tracking the data load across all files
struct LoadDataResult {
  int rawFrames = 0;
  int goodFrames = 0;
  std::vector<double> frameEventCounts;
  API::MatrixWorkspace_sptr dataWorkspace;
};

class LoadDataStrategyBase {
public:
  virtual void addEvent(double &minToF, double &maxToF, const double tof, const double binWidth,
                        const size_t pixel) = 0;
  virtual void addFrame(int &rawFrames, int &goodFrames, const int eventCountInFrame, const int minEventsReq,
                        const int maxEventsReq, MantidVec &frameEventCounts) = 0;
};

class LoadDataStrategyHisto final : public LoadDataStrategyBase {
public:
  LoadDataStrategyHisto(const int minToF, const int maxToF, const int binWidth);
  void addEvent(double &minToF, double &maxToF, const double tof, const double binWidth,
                const size_t pixel) override final;
  void addFrame(int &rawFrames, int &goodFrames, const int eventCountInFrame, const int minEventsReq,
                const int maxEventsReq, MantidVec &frameEventCounts) override;
  inline std::vector<std::vector<double>> &getCounts() { return m_counts; }
  inline std::vector<double> &getBinEdges() { return m_binEdges; }

private:
  std::vector<std::vector<double>> m_counts;
  std::vector<std::vector<double>> m_countsInFrame;
  std::vector<double> m_binEdges;
};

class LoadDataStrategyEvent final : public LoadDataStrategyBase {
public:
  LoadDataStrategyEvent();
  void addEvent(double &minToF, double &maxToF, const double tof, const double binWidth,
                const size_t pixel) override final;
  void addFrame(int &rawFrames, int &goodFrames, const int eventCountInFrame, const int minEventsReq,
                const int maxEventsReq, MantidVec &frameEventCounts) override;
  std::vector<DataObjects::EventList> &getEvents() { return m_events; }

private:
  std::vector<DataObjects::EventList> m_events;
  std::vector<DataObjects::EventList> m_eventsInFrame;
};

class MANTID_DATAHANDLING_DLL LoadNGEM : public API::IFileLoader<Kernel::FileDescriptor> {
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
  int m_fileCount = 0;
  /// Initialise the algorithm.
  void init() override;
  /// Execute the algorithm.
  void exec() override;
  /// Load a file into the event lists.
  void loadSingleFile(const std::vector<std::string> &filePath, int &eventCountInFrame, double &minToF, double &maxToF,
                      const double binWidth, int &rawFrames, int &goodFrames, const int minEventsReq,
                      const int maxEventsReq, MantidVec &frameEventCounts, const size_t totalFilePaths,
                      std::shared_ptr<LoadDataStrategyBase> strategy);
  /// Check that a file to be loaded is in 128 bit words.
  size_t verifyFileSize(std::ifstream &file);
  /// Reports progress and checks cancel flag.
  bool reportProgressAndCheckCancel(size_t &numProcessedEvents, int &eventCountInFrame, const size_t totalNumEvents,
                                    const size_t totalFilePaths);
  /// Create a workspace to store the number of counts per frame.
  void createCountWorkspace(const std::vector<double> &frameEventCounts);
  /// Load the instrument and attach to the data workspace.
  void loadInstrument(API::MatrixWorkspace_sptr &dataWorkspace);
  /// Validate the imputs into the algorithm, overrides.
  std::map<std::string, std::string> validateInputs() override;
  /// Validate events per frame inputs
  std::pair<std::string, std::string> validateEventsPerFrame();
  /// Validate minimum and maximum TOF
  std::pair<std::string, std::string> validateMinMaxToF();
  /// Read data from files into as histograms into a workspace2D
  LoadDataResult readDataAsHistograms(double &minToF, double &maxToF, const double binWidth, const int minEventsReq,
                                      const int maxEventsReq, const std::vector<std::vector<std::string>> &filePaths);
  /// Read data from files into an event workspace, preserving events
  LoadDataResult readDataAsEvents(double &minToF, double &maxToF, const double binWidth, const int minEventsReq,
                                  const int maxEventsReq, const std::vector<std::vector<std::string>> &filePaths);
};

} // namespace DataHandling
} // namespace Mantid
