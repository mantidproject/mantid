#ifndef MANTID_DATAHANDLING_LOADNEXUSMONITORS_H_
#define MANTID_DATAHANDLING_LOADNEXUSMONITORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <boost/scoped_array.hpp>
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

namespace Mantid {

namespace DataHandling {
/** @class LoadNexusMonitors LoadNexusMonitors.h
DataHandling/LoadNexusMonitors.h

Load Monitors from NeXus files.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> Workspace - The name of the workspace to output</LI>
</UL>

@author Michael Reuter, SNS
@date October 25, 2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class DLLExport LoadNexusMonitors : public API::Algorithm {
public:
  /// Default constructor
  LoadNexusMonitors();
  /// Destructor
  virtual ~LoadNexusMonitors();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadNexusMonitors"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load all monitors from a NeXus file into a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Nexus"; }

private:
  /// Intialisation code
  void init();
  /// Execution code
  void exec();

  // Fix the detector numbers if the defaults are not correct
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

  /// The name and path of the input file
  std::string filename;
  /// The workspace being filled out
  API::MatrixWorkspace_sptr WS;
  /// Number of monitors
  size_t nMonitors;
  /// name of top level NXentry to use
  std::string m_top_entry_name;
};
}
}
#endif /* MANTID_DATAHANDLING_LOADNEXUSMONITORS_H_ */
