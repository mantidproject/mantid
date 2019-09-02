// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FILTEREVENTSBYLOGVALUEPRENEXUS_H_
#define FILTEREVENTSBYLOGVALUEPRENEXUS_H_

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/BinaryFile.h"
#include <fstream>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
/** @class Mantid::DataHandling::FilterEventsByLogValuePreNexus

    A data loading and splitting routine for SNS pre-nexus event files
*/

/// This define is used to quickly turn parallel code on or off.
#undef LOADEVENTPRENEXUS_ALLOW_PARALLEL

/// Make the code clearer by having this an explicit type
using PixelType = int;

/// Type for the DAS time of flight (data file)
using DasTofType = int;

/// Structure that matches the form in the binary event list.
#pragma pack(push, 4) // Make sure the structure is 8 bytes.
struct DasEvent {
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
};
#pragma pack(pop)

/// Structure used as an intermediate for parallel processing of events
#pragma pack(push, 4) // Make sure the structure is 8 bytes.
struct IntermediateEvent {
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
  /// Frame index (pulse # of this event)
  size_t frame_index;
  /// Period of the event (not really used at this time)
  uint32_t period;
};
#pragma pack(pop)

/// Structure that matches the form in the new pulseid files.
#pragma pack(push, 4) // Make sure the structure is 16 bytes.
struct Pulse {
  /// The number of nanoseconds since the seconds field. This is not necessarily
  /// less than one second.
  uint32_t nanoseconds;

  /// The number of seconds since January 1, 1990.
  uint32_t seconds;

  /// The index of the first event for this pulse.
  uint64_t event_index;

  /// The proton charge for the pulse.
  double pCurrent;
};
#pragma pack(pop)

class DLLExport FilterEventsByLogValuePreNexus
    : public API::IFileLoader<Kernel::FileDescriptor>,
      public API::DeprecatedAlgorithm {
public:
  /// Constructor
  FilterEventsByLogValuePreNexus();
  /// Virtual destructor
  ~FilterEventsByLogValuePreNexus() override;
  /// Algorithm's name
  const std::string name() const override {
    return "FilterEventsByLogValuePreNexus";
  }
  /// Algorithm's version
  int version() const override { return (2); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\PreNexus";
  }
  /// Algorithm's aliases
  const std::string alias() const override { return "LoadEventPreNeXus2"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load and split SNS raw neutron event data format and stores it in "
           "a workspace";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Process properties
  void processProperties();

  /// Create, initialize and set up output EventWrokspace
  DataObjects::EventWorkspace_sptr setupOutputEventWorkspace();

  void loadPixelMap(const std::string &filename);

  void openEventFile(const std::string &filename);

  void readPulseidFile(const std::string &filename, const bool throwError);

  void runLoadInstrument(const std::string &eventfilename,
                         API::MatrixWorkspace_sptr localWorkspace);

  void procEvents(DataObjects::EventWorkspace_sptr &workspace);

  void procEventsLinear(DataObjects::EventWorkspace_sptr &workspace,
                        std::vector<Types::Event::TofEvent> **arrayOfVectors,
                        DasEvent *event_buffer,
                        size_t current_event_buffer_size, size_t fileOffset);

  void setProtonCharge(DataObjects::EventWorkspace_sptr &workspace);

  void addToWorkspaceLog(std::string logtitle, size_t mindex);

  void processEventLogs();

  /// Pad out empty pixel
  size_t padOutEmptyPixels(DataObjects::EventWorkspace_sptr eventws);

  /// Set up spectrum/detector ID map inside a workspace
  void setupPixelSpectrumMap(DataObjects::EventWorkspace_sptr eventws);

  ///
  void filterEvents();
  ///
  void filterEventsLinear(DataObjects::EventWorkspace_sptr &workspace,
                          std::vector<Types::Event::TofEvent> **arrayOfVectors,
                          DasEvent *event_buffer,
                          size_t current_event_buffer_size, size_t fileOffset);

  /// Correct wrong event indexes with pulse
  void unmaskVetoEventIndexes();

  /// Use pulse index/ event index to find out the frequency of instrument
  /// running
  int findRunFrequency();

  void debugOutput(bool doit, size_t mindex);

  /// Perform statistics to event (wrong pixel ID) logs
  void doStatToEventLog(size_t mindex);

  std::unique_ptr<Mantid::API::Progress> m_progress = nullptr;

  DataObjects::EventWorkspace_sptr m_localWorkspace; //< Output EventWorkspace
  std::vector<int64_t> m_spectraList;                ///< the list of Spectra

  /// The times for each pulse.
  std::vector<Types::Core::DateAndTime> pulsetimes;
  /// The index of the first event in each pulse.
  std::vector<uint64_t> m_vecEventIndex;
  /// The proton charge on a pulse by pulse basis.
  std::vector<double> m_protonCharge;
  /// The total proton charge for the run.
  double m_protonChargeTot;
  /// The value of the vector is the workspace index. The index into it is the
  /// pixel ID from DAS
  std::vector<std::size_t> m_pixelToWkspindex;
  /// Map between the DAS pixel IDs and our pixel IDs, used while loading.
  std::vector<PixelType> m_pixelmap;

  /// The maximum detector ID possible
  Mantid::detid_t m_detid_max;

  /// Handles loading from the event file
  std::unique_ptr<Mantid::Kernel::BinaryFile<DasEvent>> m_eventFile;
  std::size_t m_numEvents; ///< The number of events in the file
  std::size_t m_numPulses; ///< the number of pulses
  uint32_t m_numPixel;     ///< the number of pixels

  std::size_t m_numGoodEvents;  ///< The number of good events loaded
  std::size_t m_numErrorEvents; ///< The number of error events encountered
  std::size_t m_numBadEvents;   ///< The number of bad events. Part of error
                                ///< events
  std::size_t m_numWrongdetidEvents; ///< The number of events with wrong
  /// detector IDs. Part of error events.
  std::set<PixelType> wrongdetids; ///< set of all wrong detector IDs
  std::map<PixelType, size_t> wrongdetidmap;
  std::vector<std::vector<Types::Core::DateAndTime>> wrongdetid_pulsetimes;
  std::vector<std::vector<double>> wrongdetid_tofs;

  /// the number of events that were ignored (not loaded) because, e.g. of only
  /// loading some spectra.
  std::size_t m_numIgnoredEvents;
  std::size_t m_firstEvent;   ///< The first event to load (count from zero)
  std::size_t m_maxNumEvents; ///< Number of events to load

  /// Set to true if a valid Mapping file was provided.
  bool m_usingMappingFile;

  /// For loading only some spectra
  bool m_loadOnlySomeSpectra;
  /// Handle to the loaded spectra map
  std::map<int64_t, bool> spectraLoadMap;

  /// Longest TOF limit
  double m_longestTof;
  /// Shortest TOF limit
  double m_shortestTof;

  /// Flag to allow for parallel loading
  bool m_parallelProcessing;

  /// Whether or not the pulse times are sorted in increasing order.
  bool m_pulseTimesIncreasing;

  /// sample environment event
  std::vector<detid_t> mSEids;
  std::map<size_t, detid_t> mSEmap;
  std::vector<std::vector<int64_t>> mSEpulseids;
  std::vector<std::vector<double>> mSEtofs;

  /// Event file
  std::string m_eventFileName;

  /// Pulse ID file
  std::string m_pulseIDFileName;
  /// Throw error with bad pulse ID
  bool m_throwError;

  /// Function mode
  std::string m_functionMode;

  /// Flag for examine event (log)
  bool m_examEventLog;

  /// Pixel ID to exam
  int m_pixelid2exam;

  /// Number of events to write out
  int m_numevents2write;

  /// Log pixel IDs for filtering
  std::vector<int> m_vecLogPixelID;
  /// Log pixel Tags for filtering
  std::vector<std::string> m_vecLogPixelTag;

  /// Output EventWorkspace for filtered event B->A
  DataObjects::EventWorkspace_sptr m_localWorkspaceBA;

  /// Accelerator operation frequency
  int m_freqHz;

  int64_t m_istep;

  int64_t m_dbPixelID;
  bool m_useDBOutput;

  bool m_corretctTOF;
};
}
} // namespace Mantid
#endif /*FILTEREVENTSBYLOGVALUEPRENEXUS_H_*/
