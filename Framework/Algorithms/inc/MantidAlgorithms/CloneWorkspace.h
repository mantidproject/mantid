// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CLONEWORKSPACE_H_
#define MANTID_ALGORITHMS_CLONEWORKSPACE_H_

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/** Creates a copy of the input workspace. At the moment, this is only available
   for MatrixWorkspaces, though
    it should be perfectly possible to extend this to include TableWorkspaces if
   that is ever required.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 09/12/2009
*/
class DLLExport CloneWorkspace : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CloneWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Copies an existing workspace into a new one.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CompareWorkspaces"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  const std::string workspaceMethodName() const override { return "clone"; }
  const std::string workspaceMethodInputProperty() const override {
    return "InputWorkspace";
  }

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CLONEWORKSPACE_H_*/
