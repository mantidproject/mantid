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
   </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> XMin - The X value to start the cropped workspace at (default 0)</LI>
    <LI> XMax - The X value to end the cropped workspace at (default max)</LI>
    <LI> StartSpectrum - The workspace index number to start the cropped
   workspace from (default 0)</LI>
    <LI> EndSpectrum - The workspace index number to end the cropped workspace
   at (default max)</LI>
    </UL>

    If the input workspace must has common bin boundaries/X values then cropping
   in X will
    lead to an output workspace with fewer bins than the input one. If the
   boundaries are
    not common then the output workspace will have the same number of bins as
   the input one,
    but with data values outside the X range given set to zero.
    If an X value given does not exactly match a bin boundary, then the closest
   bin boundary
    within the range will be used.
    Note that if none of the optional properties are given, then the output
   workspace will be
    a copy of the input one.

    @author Russell Taylor, Tessella Support Services plc
    @date 15/10/2008
*/
class MANTID_ALGORITHMS_DLL CropWorkspace : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CropWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Extracts a 'block' from a workspace and places it in a new "
           "workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspaceRagged", "CropToComponent", "RemoveBins", "ExtractSingleSpectrum", "ExtractSpectra"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Splitting"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
