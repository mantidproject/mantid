// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADRAWHELPER_H_
#define MANTID_DATAHANDLING_LOADRAWHELPER_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidDataObjects/Workspace2D.h"
#include <climits>
#include <memory>

class ISISRAW;
class ISISRAW2;

namespace Mantid {
namespace API {
class SpectrumDetectorMapping;
}

namespace DataHandling {
/** @class LoadRawHelper DataHandling/LoadRawHelper.h

Helper class for LoadRaw algorithms.


@author Sofia Antony, ISIS,RAL
@date 14/04/2010
*/
class DLLExport LoadRawHelper
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Default constructor
  LoadRawHelper();
  // Define destructor in .cpp as we have unique_ptr to forward declared
  // ISISRAW2
  ~LoadRawHelper();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadRawHelper"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Helper class for LoadRaw algorithms.";
  }
  /// Opens Raw File
  FILE *openRawFile(const std::string &fileName);
  /// Read in run parameters Public so that LoadRaw2 can use it
  void loadRunParameters(API::MatrixWorkspace_sptr localWorkspace,
                         ISISRAW *const = nullptr) const;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

  /// returns true if the Exclude Monitor option(property) selected
  static bool isExcludeMonitors(const std::string &monitorOption);
  ///  returns true if the Separate Monitor Option  selected
  static bool isSeparateMonitors(const std::string &monitorOption);
  ///  returns true if the Include Monitor Option  selected
  static bool isIncludeMonitors(const std::string &monitorOption);

  static void ProcessLoadMonitorOptions(bool &bincludeMonitors,
                                        bool &bseparateMonitors,
                                        bool &bexcludeMonitors,
                                        API::Algorithm *const pAlgo);
  /// creates monitor workspace
  static void
  createMonitorWorkspace(DataObjects::Workspace2D_sptr &monws_sptr,
                         DataObjects::Workspace2D_sptr &normalws_sptr,
                         API::WorkspaceGroup_sptr &mongrp_sptr,
                         const int64_t mwsSpecs, const int64_t nwsSpecs,
                         const int64_t numberOfPeriods, const int64_t lengthIn,
                         std::string title, API::Algorithm *const pAlg);
  /// creates  shared pointer to group workspace
  static API::WorkspaceGroup_sptr createGroupWorkspace();

  /// creates shared pointer to workspace from parent workspace
  static DataObjects::Workspace2D_sptr
  createWorkspace(DataObjects::Workspace2D_sptr ws_sptr, int64_t nVectors = -1,
                  int64_t xLengthIn = -1, int64_t yLengthIn = -1);

  /// overloaded method to create shared pointer to workspace
  static DataObjects::Workspace2D_sptr
  createWorkspace(int64_t nVectors, int64_t xlengthIn, int64_t ylengthIn,
                  const std::string &title);

  /// sets the workspace property
  static void setWorkspaceProperty(const std::string &propertyName,
                                   const std::string &title,
                                   API::WorkspaceGroup_sptr grpws_sptr,
                                   DataObjects::Workspace2D_sptr ws_sptr,
                                   int64_t numberOfPeriods, bool bMonitor,
                                   API::Algorithm *const pAlg);

  /// overloaded method to set the workspace property
  static void setWorkspaceProperty(DataObjects::Workspace2D_sptr ws_sptr,
                                   API::WorkspaceGroup_sptr grpws_sptr,
                                   const int64_t period, bool bmonitors,
                                   API::Algorithm *const pAlg);

  /// Extract the start time from a raw file
  static Types::Core::DateAndTime extractStartTime(ISISRAW &isisRaw);

  /// Extract the end time from a raw file
  static Types::Core::DateAndTime extractEndTime(ISISRAW &isisRaw);

protected:
  /// Overwrites Algorithm method.
  void init() override;
  /// checks the file is an ascii file
  bool isAscii(FILE *file) const;
  /// Reads title from the isisraw class
  void readTitle(FILE *file, std::string &title);
  /// reads workspace parameters like number of histograms,size of vectors etc
  void readworkspaceParameters(specnum_t &numberOfSpectra, int &numberOfPeriods,
                               int64_t &lengthIn, int64_t &noTimeRegimes);

  /// skips histrogram data from raw file.
  void skipData(FILE *file, int hist);
  void skipData(FILE *file, int64_t hist);

  /// calls isisRaw ioraw
  void ioRaw(FILE *file, bool from_file);

  /// reads data
  bool readData(FILE *file, int hist);
  bool readData(FILE *file, int64_t hist);

  // Constructs the time channel (X) vector(s)
  std::vector<boost::shared_ptr<HistogramData::HistogramX>>
  getTimeChannels(const int64_t &regimes, const int64_t &lengthIn);
  /// loadinstrument Child Algorithm
  void runLoadInstrument(const std::string &fileName,
                         DataObjects::Workspace2D_sptr, double, double);
  /// loadinstrumentfromraw algorithm
  void runLoadInstrumentFromRaw(const std::string &fileName,
                                DataObjects::Workspace2D_sptr);
  /// loadinstrumentfromraw Child Algorithm
  void runLoadMappingTable(const std::string &fileName,
                           DataObjects::Workspace2D_sptr);
  /// load log algorithm
  void runLoadLog(const std::string &fileName, DataObjects::Workspace2D_sptr,
                  double, double);

  /// Create the period specific logs
  void createPeriodLogs(int64_t period,
                        DataObjects::Workspace2D_sptr local_workspace);

  /// gets the monitor spectrum list from the workspace
  std::vector<specnum_t>
  getmonitorSpectrumList(const API::SpectrumDetectorMapping &mapping);

  /// This method sets the raw file data to workspace vectors
  void setWorkspaceData(
      DataObjects::Workspace2D_sptr newWorkspace,
      const std::vector<boost::shared_ptr<HistogramData::HistogramX>>
          &timeChannelsVec,
      int64_t wsIndex, specnum_t nspecNum, int64_t noTimeRegimes,
      int64_t lengthIn, int64_t binStart);

  /// get proton charge from raw file
  float getProtonCharge() const;
  /// set proton charge
  void setProtonCharge(API::Run &run);
  /// Stores the run number in the sample's logs
  void setRunNumber(API::Run &run);

  /// number of time regimes
  int getNumberofTimeRegimes();
  /// return an reference to the ISISRAW2 reader
  ISISRAW2 &isisRaw() const;
  /// resets the isisraw shared pointer
  void reset();

  /// sets optional properties like spec_min,spec_max etc
  void setOptionalProperties(const int &spec_min, const int &spec_max,
                             const std::vector<int> &spec_list);
  /// Validates the optional 'spectra to read' properties, if they have been set
  void checkOptionalProperties();
  /// calculate workspace size
  specnum_t calculateWorkspaceSize();
  /// calculate workspace sizes if separate or exclude monitors are selected
  void calculateWorkspacesizes(const std::vector<specnum_t> &monitorSpecList,
                               specnum_t &normalwsSpecs,
                               specnum_t &monitorwsSpecs);
  /// load the spectra
  void loadSpectra(FILE *file, const int &period, const int &total_specs,
                   DataObjects::Workspace2D_sptr ws_sptr,
                   std::vector<boost::shared_ptr<HistogramData::HistogramX>>);

  /// Has the spectrum_list property been set?
  bool m_list;
  /// Have the spectrum_min/max properties been set?
  bool m_interval;
  /// The value of the spectrum_list property
  std::vector<specnum_t> m_spec_list;
  /// The value of the spectrum_min property
  specnum_t m_spec_min;
  /// The value of the spectrum_max property
  specnum_t m_spec_max;
  /// The number of periods in the raw file
  int m_numberOfPeriods;

private:
  /// Overwrites Algorithm method
  void exec() override;
  /// convert month label to int string
  static std::string convertMonthLabelToIntStr(std::string month);

  /// ISISRAW class instance which does raw file reading.
  mutable std::unique_ptr<ISISRAW2> m_isis_raw;
  /// Allowed values for the cache property
  std::vector<std::string> m_cache_options;
  /// A map for storing the time regime for each spectrum
  std::map<specnum_t, specnum_t> m_specTimeRegimes;
  /// The current value of the progress counter
  double m_prog;

  /// number of spectra
  specnum_t m_numberOfSpectra;

  /// a vector holding the indexes of monitors
  std::vector<specnum_t> m_monitordetectorList;

  /// boolean for list spectra options
  bool m_bmspeclist;

  /// the total nuumber of spectra
  specnum_t m_total_specs;

  /// A ptr to the log creator
  std::unique_ptr<ISISRunLogs> m_logCreator;

  /// Search for the log files in the workspace, and output their names as a
  /// set.
  std::list<std::string> searchForLogFiles(const std::string &pathToRawFile);
  /// Extract the log name from the path to the specific log file.
  std::string extractLogName(const std::string &path);
  /// Checks if the file is an ASCII file
  bool isAscii(const std::string &filename);
  /// if  alternate data stream named checksum exists for the raw file
  bool adsExists(const std::string &pathToFile);
  /// returns the list of log files from ADS checksum
  std::set<std::string>
  getLogFilenamesfromADS(const std::string &pathToRawFile);
};

} // namespace DataHandling
} // namespace Mantid
#endif /*MANTID_DATAHANDLING_LOADRAW_H_*/
