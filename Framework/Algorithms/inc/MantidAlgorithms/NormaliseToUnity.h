// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Takes a workspace as input and normalises it to 1.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. Must be a
   histogram. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> RangeLower - The X value to integrate from (default 0)</LI>
    <LI> RangeUpper - The X value to integrate to (default max)</LI>
    <LI> StartWorkspaceIndex - Workspace index number to integrate from (default
   0)</LI>
    <LI> EndWorkspaceIndex - Workspace index number to integrate to (default
   max)</LI>
    <LI> IncludePartialBins - If true then partial bins from the beginning and
   end of the input range are also included in the integration (default
   false)</LI>
    <LI> IncludeMonitors - Whether to include monitor spectra in the sum
   (default yes)
    </UL>
 */
class MANTID_ALGORITHMS_DLL NormaliseToUnity : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "NormaliseToUnity"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "NormaliseToUnity takes a 2D workspace or an EventWorkspace as "
           "input and normalises it to 1. Optionally, the range summed can be "
           "restricted in either dimension.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Divide"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions\\NormalisationCorrections"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
