// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileDescriptor.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace DataHandling {
/** @class LoadCanSAS1D  DataHandling/LoadCanSAS1D.h

This algorithm loads 1 CanSAS1d xml file into a workspace.

Required properties:
<UL>
<LI> OutputWorkspace - The name of workspace to be created.</LI>
<LI> Filename - Name of the file to load</LI>
</UL>

@author Sofia Antony, Rutherford Appleton Laboratory
@date 26/01/2010
*/
class DLLExport LoadCanSAS1D : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadCanSAS1D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load a file written in the canSAS 1-D data format"; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\XML;SANS\\DataHandling"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

protected:
  /// If a workspace group is created this is set from empty to the root name of
  /// the members, the name of the workspace group members up to and including
  /// the _
  std::string m_groupMembersBase;
  /// When a workspace group is being written this is the number of the last
  /// member that was written
  int m_groupNumber = 0;

  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// Loads an individual SASentry element into a new workspace
  virtual API::MatrixWorkspace_sptr loadEntry(Poco::XML::Node *const workspaceData, std::string &runName);
  /// Checks if the pointer to the loaded data is not null or throws if it is
  void check(const Poco::XML::Element *const toCheck, const std::string &name) const;
  /// Appends the new data workspace creating a workspace group if there was
  /// existing data
  void appendDataToOutput(const API::MatrixWorkspace_sptr &newWork, const std::string &newWorkName,
                          const API::WorkspaceGroup_sptr &container);
  /// Run LoadInstrument Child Algorithm
  void runLoadInstrument(const std::string &inst_name, const API::MatrixWorkspace_sptr &localWorkspace);
  /// Loads data into the run log
  void createLogs(const Poco::XML::Element *const sasEntry, const API::MatrixWorkspace_sptr &wSpace) const;
  /// Loads the information about hhe sample
  void createSampleInformation(const Poco::XML::Element *const sasEntry,
                               const Mantid::API::MatrixWorkspace_sptr &wSpace) const;
};
} // namespace DataHandling
} // namespace Mantid
