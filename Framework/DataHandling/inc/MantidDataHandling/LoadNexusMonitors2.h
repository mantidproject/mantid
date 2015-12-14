#ifndef MANTID_DATAHANDLING_LOADNEXUSMONITORS2_H_
#define MANTID_DATAHANDLING_LOADNEXUSMONITORS2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <boost/scoped_array.hpp>
#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>

namespace Mantid {
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
class DLLExport LoadNexusMonitors2 : public API::Algorithm {
public:
  /// Default constructor
  LoadNexusMonitors2();

  /// Destructor
  virtual ~LoadNexusMonitors2();

  /// Algorithm's name for identification
  virtual const std::string name() const override {
    return "LoadNexusMonitors";
  }

  /// Summary of algorithms purpose
  virtual const std::string summary() const override {
    return "Load all monitors from a NeXus file into a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const override { return 2; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const override {
    return "DataHandling\\Nexus";
  }

protected:
  /// Initialise algorithm
  virtual void init() override;

  /// Execute algorithm
  virtual void exec() override;

private:
  /// Fix the detector numbers if the defaults are not correct
  void fixUDets(boost::scoped_array<detid_t> &det_ids, ::NeXus::File &file,
                const boost::scoped_array<specid_t> &spec_ids,
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

private:
  std::string m_filename; ///< The name and path of the input file
  API::MatrixWorkspace_sptr m_workspace; ///< The workspace being filled out
  size_t m_monitor_count;                ///< Number of monitors
  std::string m_top_entry_name;          ///< name of top level NXentry to use
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADNEXUSMONITORS2_H_
