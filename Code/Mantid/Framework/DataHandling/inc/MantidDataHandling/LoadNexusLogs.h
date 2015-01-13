#ifndef MANTID_DATAHANDLING_LOADNEXUSLOGS_H_
#define MANTID_DATAHANDLING_LOADNEXUSLOGS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace Kernel {
class Property;
}
namespace API {
class MatrixWorkspace;
}

namespace DataHandling {

/**

Loads the run logs from a NeXus file.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> Workspace - The name of the workspace in which to store the imported
data.</LI>
</UL>

@author Martyn Gigg, Tessella plc

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadNexusLogs : public API::Algorithm {
public:
  /// Default constructor
  LoadNexusLogs();
  /// Destructor
  virtual ~LoadNexusLogs() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadNexusLogs"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads run logs (temperature, pulse charges, etc.) from a NeXus "
           "file and adds it to the run information in a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\Logs;DataHandling\\Nexus";
  }

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();
  /// Load log data from a group
  void loadLogs(::NeXus::File &file, const std::string &entry_name,
                const std::string &entry_class,
                boost::shared_ptr<API::MatrixWorkspace> workspace) const;
  /// Load an NXlog entry
  void loadNXLog(::NeXus::File &file, const std::string &entry_name,
                 const std::string &entry_class,
                 boost::shared_ptr<API::MatrixWorkspace> workspace) const;
  /// Load an IXseblock entry
  void loadSELog(::NeXus::File &file, const std::string &entry_name,
                 boost::shared_ptr<API::MatrixWorkspace> workspace) const;
  void loadVetoPulses(::NeXus::File &file,
                      boost::shared_ptr<API::MatrixWorkspace> workspace) const;

  /// Create a time series property
  Kernel::Property *createTimeSeries(::NeXus::File &file,
                                     const std::string &prop_name) const;
  /// Progress reporting object
  boost::shared_ptr<API::Progress> m_progress;

  /// Use frequency start for Monitor19 and Special1_19 logs with "No Time" for
  /// SNAP
  std::string freqStart;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUSLOGS_H_*/
