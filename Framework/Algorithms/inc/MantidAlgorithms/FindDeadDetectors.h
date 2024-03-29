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
/**
Takes a workspace as input and identifies all of the spectra that have a
value across all time bins less or equal to than the threshold 'dead' value.
This is then used to mark all 'dead' detectors with a 'dead' marker value,
while all spectra from live detectors are given a value of 'live' marker value.

This is primarily used to ease identification using the instrument visualization
tools.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
</UL>

Optional Properties:
<UL>
<LI> DeadThreshold - The threshold against which to judge if a spectrum belongs
to a dead detector (default 0.0)</LI>
<LI> LiveValue - The value to assign to an integrated spectrum flagged as 'live'
(default 0.0)</LI>
<LI> DeadValue - The value to assign to an integrated spectrum flagged as 'dead'
(default 100.0)</LI>
<LI> StartX - Start the integration at the above bin above the one that this
value is in (default: the start of each histogram)</LI>
<LI> EndX - Stop the integration at the bin before the one that contains this x
value (default: the end of each histogram)</LI>
<LI> OutputFile - (Optional) A filename to which to write the list of dead
detector UDETs </LI>
</UL>

@author Nick Draper, Tessella Support Services plc
@date 02/10/2008
*/
class MANTID_ALGORITHMS_DLL FindDeadDetectors final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FindDeadDetectors"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Identifies and flags empty spectra caused by 'dead' detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FindDetectorsOutsideLimits", "DetectorDiagnostic"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diagnostics"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  API::MatrixWorkspace_sptr integrateWorkspace();
};

} // namespace Algorithms
} // namespace Mantid
