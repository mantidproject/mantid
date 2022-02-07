// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Extracts a 'block' from a workspace and places it in a new workspace.
    (Or, to look at it another way, lops bins or spectra off a workspace).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
    <LI> XMin - The X values to start the cropping the spectra at</LI>
    <LI> XMax - The X values to end the cropping the spectra at</LI>
    </UL>

*/
class MANTID_ALGORITHMS_DLL CropWorkspaceRagged : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CropWorkspaceRagged"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Crops each spectrum of a workspace and produces a new "
           "workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"CropWorkspace"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Splitting"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Algorithms
} // namespace Mantid
