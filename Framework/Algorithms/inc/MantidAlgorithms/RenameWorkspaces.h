// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RENAMEWORKSPACES_H_
#define MANTID_ALGORITHMS_RENAMEWORKSPACES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Renames a workspace to a different name in the data service.
    If the same name is provided for input and output then the algorithm will
   fail with an error.
    The renaming is implemented as a removal of the original workspace from the
   data service
    and re-addition under the new name.

    Required Properties:
    <UL>
    <LI> InputWorkspace - Comma separated list of names of the Workspace to take
   as input </LI>
    <LI> OutputWorkspace - string to append or prefix of the names of the
   workspaces </LI>
    <LI> Prefix - true if prefixing the Output Workspace name
    </UL>
 */
class DLLExport RenameWorkspaces : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RenameWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rename the Workspace. Please provide either a comma-separated list "
           "of new workspace names in WorkspaceNames or Prefix and/or Suffix "
           "to add to existing names";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"RenameWorkspace"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Utility\\Workspaces"; }

  /// Validator to check out name does not already exist
  std::map<std::string, std::string> validateInputs() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RENAMEWORKSPACES_H_*/
