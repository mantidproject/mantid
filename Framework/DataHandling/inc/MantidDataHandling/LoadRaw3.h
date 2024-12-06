// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid {
namespace DataHandling {
/** @class LoadRaw3 LoadRaw3.h DataHandling/LoadRaw3.h

Loads an file in ISIS RAW format and stores it in a 2D workspace
(Workspace2D class). LoadRaw is an algorithm and LoadRawHelper class and
overrides the init() & exec() methods.
LoadRaw3 uses less memory by only loading up the datablocks as required.
 */
class MANTID_DATAHANDLING_DLL LoadRaw3 : public LoadRawHelper {

public:
  /// Default constructor
  LoadRaw3();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadRaw"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a data file in ISIS  RAW format and stores it in a 2D "
           "workspace (Workspace2D class).";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 3; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadVesuvio", "RawFileInfo", "LoadSampleDetailsFromRaw", "LoadRawBin0", "LoadRawSpectrum0"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Raw"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// returns true if the given spectrum is a monitor
  bool isMonitor(const std::vector<specnum_t> &monitorIndexes, specnum_t spectrumNum);

  /// validate workspace sizes
  void validateWorkspaceSizes(bool bexcludeMonitors, bool bseparateMonitors, const int64_t normalwsSpecs,
                              const int64_t monitorwsSpecs);

  /// creates output workspace, monitors excluded from this workspace
  void excludeMonitors(FILE *file, const int &period, const std::vector<specnum_t> &monitorList,
                       const DataObjects::Workspace2D_sptr &ws_sptr);

  /// creates output workspace whcih includes monitors
  void includeMonitors(FILE *file, const int64_t &period, const DataObjects::Workspace2D_sptr &ws_sptr);

  /// creates two output workspaces none normal workspace and separate one for
  /// monitors
  void separateMonitors(FILE *file, const int64_t &period, const std::vector<specnum_t> &monitorList,
                        const DataObjects::Workspace2D_sptr &ws_sptr, const DataObjects::Workspace2D_sptr &mws_sptr);

  /// skip all spectra in a period
  void skipPeriod(FILE *file, const int64_t &period);
  /// return true if loading a selection of periods
  bool isSelectedPeriods() const { return !m_periodList.empty(); }
  /// check if a period should be loaded
  bool isPeriodIncluded(int period) const;
  /// get the previous period number
  int getPreviousPeriod(int period) const;

  /// sets optional properties
  void setOptionalProperties();

  /// sets progress taking account of progress time taken up by ChildAlgorithms
  void setProg(double);

  /// The name and path of the input file
  std::string m_filename;

  /// The number of spectra in the raw file
  specnum_t m_numberOfSpectra;
  /// Allowed values for the cache property
  std::vector<std::string> m_cache_options;
  /// A map for storing the time regime for each spectrum
  std::map<int64_t, int64_t> m_specTimeRegimes;
  /// number of time regime
  int64_t m_noTimeRegimes;
  /// The current value of the progress counter
  double m_prog;
  /// Start and ends values of progress counter
  double m_prog_start;
  double m_prog_end;

  /// Read in the time bin boundaries
  int64_t m_lengthIn;
  /// time channels vector
  std::vector<std::shared_ptr<HistogramData::HistogramX>> m_timeChannelsVec;
  /// total number of specs
  int64_t m_total_specs;
  /// A list of periods to read. Each value is between 1 and m_numberOfPeriods
  std::vector<int> m_periodList;
};

} // namespace DataHandling
} // namespace Mantid
