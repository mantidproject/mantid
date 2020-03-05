// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADNEXUSLOGS_H_
#define MANTID_DATAHANDLING_LOADNEXUSLOGS_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid {
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
*/
class DLLExport LoadNexusLogs : public API::DistributedAlgorithm {
public:
  /// Default constructor
  LoadNexusLogs();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadNexusLogs"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads run logs (temperature, pulse charges, etc.) from a NeXus "
           "file and adds it to the run information in a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadLog", "MergeLogs"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Logs;DataHandling\\Nexus";
  }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
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
  void loadNPeriods(::NeXus::File &file,
                    boost::shared_ptr<API::MatrixWorkspace> workspace) const;

  /// Progress reporting object
  boost::shared_ptr<API::Progress> m_progress;

  /// Use frequency start for Monitor19 and Special1_19 logs with "No Time" for
  /// SNAP
  std::string freqStart;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUSLOGS_H_*/
