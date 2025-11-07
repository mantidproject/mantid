// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TimeSplitter.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {

namespace Kernel {
class TimeROI; // forward declaration
}

namespace API {
class TimeAtSampleStrategy;
}

namespace Algorithms {

/** FilterEvents : Filter Events in EventWorkspace to multiple EventsWorkspace
  by Splitters

  @date 2012-04-04
*/
class MANTID_ALGORITHMS_DLL FilterEvents final : public API::Algorithm {

  enum TOFCorrectionType { NoneCorrect, CustomizedCorrect, DirectCorrect, ElasticCorrect, IndirectCorrect };
  enum EVENTFILTERSKIP { EventFilterSkipNoDet, EventFilterSkipNoDetTOFCorr };

public:
  FilterEvents();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FilterEvents"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Filter events from an EventWorkspace to one or multiple target "
           "event workspaces according to a series of splitters.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"GenerateEventsFilter", "FilterByTime", "FilterByLogValue"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events\\EventFiltering"; }
  std::map<std::string, std::string> validateInputs() override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  // Implement abstract Algorithm methods
  void exec() override;

  /// Process user input properties
  void processAlgorithmProperties();

  /// process splitters specified by an input workspace
  void parseInputSplitters();

  /// create output workspaces
  void createOutputWorkspaces();

  /// Set up detector calibration parameters
  void setupDetectorTOFCalibration();

  /// Set up detector calibration parameters for elastic scattering instrument
  API::TimeAtSampleStrategy *setupElasticTOFCorrection() const;

  /// Set up detector calibration parmaeters for direct inelastic scattering
  /// instrument
  API::TimeAtSampleStrategy *setupDirectTOFCorrection() const;

  /// Set up detector calibration parameters for indirect inelastic scattering
  /// instrument
  API::TimeAtSampleStrategy *setupIndirectTOFCorrection() const;

  /// Set up detector calibration parameters from customized values
  void setupCustomizedTOFCorrection();

  /// Filter events by splitters in format of Splitter
  void filterEvents(double progressamount);

  /// Mark event lists of workspace indexes with no associated detector pixels as not to be split
  void examineEventWS();

  void groupOutputWorkspace();

  /// Find the TimeROI associated to a particular destination-workspace index
  Kernel::TimeROI partialROI(const int &index);

  DataObjects::EventWorkspace_sptr m_eventWS;
  DataObjects::SplittersWorkspace_sptr m_splittersWorkspace;
  DataObjects::TableWorkspace_sptr m_splitterTableWorkspace;
  API::MatrixWorkspace_sptr m_matrixSplitterWS;
  DataObjects::TableWorkspace_sptr m_detCorrectWorkspace;

  DataObjects::TimeSplitter m_timeSplitter;
  std::set<int> m_targetWorkspaceIndexSet;

  std::map<int, DataObjects::EventWorkspace_sptr> m_outputWorkspacesMap;
  std::vector<std::string> m_wsNames;

  std::vector<double> m_detTofOffsets;
  std::vector<double> m_detTofFactors;

  bool m_filterByPulseTime;

  DataObjects::TableWorkspace_sptr m_informationWS;
  bool m_hasInfoWS;

  double m_progress;

  /// Base of output workspace's name
  std::string m_outputWSNameBase;

  /// Flag to group workspace
  bool m_toGroupWS;

  /// Vector for splitting time
  /// FIXME - shall we convert this to DateAndTime???.  Need to do speed test!
  std::vector<int64_t> m_vecSplitterTime;
  /// Vector for splitting group
  std::vector<int> m_vecSplitterGroup;

  /// TOF detector/sample correction type
  TOFCorrectionType m_tofCorrType;

  /// Spectrum skip type
  EVENTFILTERSKIP m_specSkipType;
  /// Vector for skip information
  std::vector<bool> m_vecSkip;

  // Flag to have relative time in splitters workspace
  bool m_isSplittersRelativeTime;
  // Absolute base time for relative times in splitters workspace
  Types::Core::DateAndTime m_filterStartTime;
};

} // namespace Algorithms
} // namespace Mantid
