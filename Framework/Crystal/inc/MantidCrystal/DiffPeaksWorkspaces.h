// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {
/** An algorithm that subtracts from a workspace (the LHSWorkspace) any peaks
   that match
    entries in a second workspace (the RHSWorkspace). Such peaks are those that
   are within
    the given tolerance in all components of Q. Note that a peak in the
   RHSWorkspace will
    only be matched to the first in the LHSWorkspace that is within tolerance.
*/
class MANTID_CRYSTAL_DLL DiffPeaksWorkspaces : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes from a workspace any peaks that match a peak in a second "
           "workspace.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"CreatePeaksWorkspace", "CombinePeaksWorkspaces"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
