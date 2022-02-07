// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"

namespace Mantid {
namespace Algorithms {
/**

  This algorithm implements a "bleed" diagnostic for PSD detectors (i.e. long
  tube-based detectors).
  Required Properties:
  <UL>
  <LI> InputWorkspace  - The name of the input workspace. </LI>
  <LI> OutputMaskWorkspace - The name of the output workspace. Can be the same
  as the input one. </LI>
  <LI> MaxTubeRate - The maximum rate allowed for a tube. </LI>
  <LI> NIgnoredCentralPixels - The number of pixels about the centre to ignore.
  </LI>
  </UL>

  @author Martyn Gigg, Tessella plc
  @date 2011-01-10
*/
class MANTID_ALGORITHMS_DLL CreatePSDBleedMask : public DetectorDiagnostic {
public:
  /// Default constructor
  CreatePSDBleedMask();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CreatePSDBleedMask"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Runs a diagnostic test for saturation of PSD tubes and creates a "
           "MaskWorkspace marking the failed tube spectra.";
  }
  const std::vector<std::string> seeAlso() const override { return {"IdentifyNoisyDetectors"}; }
  const std::string category() const override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Process a tube
  bool performBleedTest(const std::vector<int> &tubeIndices, const API::MatrixWorkspace_const_sptr &inputWS,
                        double maxRate, int numIgnoredPixels);
  /// Mask a tube with the given workspace indices
  void maskTube(const std::vector<int> &tubeIndices, const API::MatrixWorkspace_sptr &workspace);
};

} // namespace Algorithms
} // namespace Mantid
