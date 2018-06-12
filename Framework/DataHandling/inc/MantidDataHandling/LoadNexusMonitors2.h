#ifndef MANTID_DATAHANDLING_LOADNEXUSMONITORS2_H_
#define MANTID_DATAHANDLING_LOADNEXUSMONITORS2_H_

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include <boost/scoped_array.hpp>
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

namespace Mantid {

namespace HistogramData {
class Counts;
class BinEdges;
}
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
* Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
* National Laboratory & European Spallation Source
*
* This file is part of Mantid.
*
* Mantid is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* Mantid is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class DLLExport LoadNexusMonitors2 : public API::ParallelAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadNexusMonitors"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load all monitors from a NeXus file into a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

protected:
  /// Initialise algorithm
  void init() override;

  /// Execute algorithm
  void exec() override;

private:
  /// Fix the detector numbers if the defaults are not correct
  void fixUDets(boost::scoped_array<Mantid::detid_t> &det_ids,
                ::NeXus::File &file,
                const boost::scoped_array<Mantid::specnum_t> &spec_ids,
                const size_t nmonitors) const;

  /// Load the logs
  void runLoadLogs(const std::string filename,
                   API::MatrixWorkspace_sptr localWorkspace);

  bool allMonitorsHaveHistoData(::NeXus::File &file,
                                const std::vector<std::string> &monitorNames);

  /// is it possible to open the file?
  bool canOpenAsNeXus(const std::string &fname);

  /// split multi period histogram workspace into a workspace group
  void splitMutiPeriodHistrogramData(const size_t numPeriods);

  size_t getMonitorInfo(NeXus::File &file,
                        std::vector<std::string> &monitorNames,
                        size_t &numHistMon, size_t &numEventMon,
                        size_t &numPeriods,
                        std::map<int, std::string> &monitorNumber2Name,
                        std::vector<bool> &isEventMonitors);

  bool isEventMonitor(::NeXus::File &monitorFileHandle) const;
  std::size_t sizeOfUnopenedEntry(::NeXus::File &file,
                                  const std::string &entryName) const;
  bool eventIdNotEmptyIfExists(
      ::NeXus::File &monitorFileHandle,
      std::map<std::string, std::string> const &entries) const;
  bool hasAllEventLikeAttributes(
      std::map<std::string, std::string> const &entries) const;
  bool keyExists(std::string const &key,
                 std::map<std::string, std::string> const &entries) const;

  bool
  createOutputWorkspace(size_t numHistMon, size_t numEventMon,
                        bool monitorsAsEvents,
                        std::vector<std::string> &monitorNames,
                        std::vector<bool> &isEventMonitors,
                        const std::map<int, std::string> &monitorNumber2Name,
                        std::vector<bool> &loadMonitorFlags);

  void readEventMonitorEntry(NeXus::File &file, size_t i);

  void readHistoMonitorEntry(NeXus::File &file, size_t i, size_t numPeriods);

private:
  std::vector<HistogramData::BinEdges> m_multiPeriodBinEdges;
  std::vector<HistogramData::Counts> m_multiPeriodCounts;
  std::string m_filename; ///< The name and path of the input file
  API::MatrixWorkspace_sptr m_workspace; ///< The workspace being filled out
  size_t m_monitor_count{0};             ///< Number of monitors
  std::string m_top_entry_name;          ///< name of top level NXentry to use
  bool m_allMonitorsHaveHistoData{
      false}; ///< Flag that all monitors have histogram
              /// data in the entry
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADNEXUSMONITORS2_H_
