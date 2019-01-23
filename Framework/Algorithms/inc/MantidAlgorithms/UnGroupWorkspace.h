// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_UNGROUP_H_
#define MANTID_ALGORITHM_UNGROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Takes the name of a group workspace as input and ungroups the workspaces.

    Required Property:
    <UL>
    <LI> InputWorkspace- The name of the workspace to ungroup </LI>
     </UL>

    @author Sofia Antony
    @date 21/07/2009
 */
class DLLExport UnGroupWorkspace : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "UnGroupWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes a group workspace as input and ungroups the workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"GroupWorkspaces"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Transforms\\Grouping;Utility\\Workspaces";
  }

private:
  /// Overridden Init method
  void init() override;
  /// overridden execute method
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_UNGROUP_H_*/
