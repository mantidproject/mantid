// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexus/NeXusFile.hpp"
#include <boost/scoped_array.hpp>

namespace Mantid {

namespace HistogramData {
class Counts;
class BinEdges;
} // namespace HistogramData
namespace DataHandling {

/**
 * Load Monitors from NeXus files.
 *
 * Required Properties:
 * <UL>
 *   <LI> Filename - The name of and path to the input NeXus file </LI>
 *   <LI> Workspace - The name of the workspace to output</LI>
 * </UL>
 *
 * @author Michael Reuter, SNS
 * @author Michael Hart, ISIS
 * @date December 4, 2015
 *
 *
 */

namespace LoadNexusMonitorsAlg {
// spectrum  is implicit in the structure as index + 1 - TODO was spectrumNo
struct MonitorInfo {
  std::string name{""}; ///< name of the group in the nexus file - TODO was
  /// monitorName
  detid_t detNum{0}; ///< detector number for monitor - TODO was monIndex
  specnum_t specNum{0};
  bool hasEvent{false};
  bool hasHisto{false};
};
} // namespace LoadNexusMonitorsAlg

class MANTID_DATAHANDLING_DLL LoadNexusMonitors2 : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadNexusMonitors"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load all monitors from a NeXus file into a workspace."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

protected:
  /// Initialise algorithm
  void init() override;

  /// Execute algorithm
  void exec() override;

private:
  /// Fix the detector numbers if the defaults are not correct
  void fixUDets(::NeXus::File &file);

  /// Load the logs
  void runLoadLogs(const std::string &filename, const API::MatrixWorkspace_sptr &localWorkspace);

  /// is it possible to open the file?
  bool canOpenAsNeXus(const std::string &fname);

  /// split multi period histogram workspace into a workspace group
  void splitMutiPeriodHistrogramData(const size_t numPeriods);

  size_t getMonitorInfo(::NeXus::File &file, size_t &numPeriods);

  bool createOutputWorkspace(std::vector<bool> &loadMonitorFlags);

  void readEventMonitorEntry(::NeXus::File &file, size_t ws_index);

  void readHistoMonitorEntry(::NeXus::File &file, size_t ws_index, size_t numPeriods);

private:
  std::vector<LoadNexusMonitorsAlg::MonitorInfo> m_monitorInfo;
  std::vector<HistogramData::BinEdges> m_multiPeriodBinEdges;
  std::vector<HistogramData::Counts> m_multiPeriodCounts;
  std::string m_filename;                ///< The name and path of the input file
  API::MatrixWorkspace_sptr m_workspace; ///< The workspace being filled out
  size_t m_monitor_count{0};             ///< Number of monitors
  std::string m_top_entry_name;          ///< name of top level NXentry to use
};

} // namespace DataHandling
} // namespace Mantid
