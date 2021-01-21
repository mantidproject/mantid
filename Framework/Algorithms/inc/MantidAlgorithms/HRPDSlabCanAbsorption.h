// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
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
/** Calculates attenuation due to absorption and scattering in an HRPD 'slab'
  can

  Properties:
  <UL>
  <LI> InputWorkspace  - The name of the input workspace. </LI>
  <LI> OutputWorkspace - The name of the output workspace. Can be the same as
  the input one. </LI>
  <LI> SampleAttenuationXSection - The attenuation cross-section for the sample
  material in barns. </LI>
  <LI> SampleScatteringXSection - The scattering cross-section for the sample
  material in barns. </LI>
  <LI> SampleNumberDensity - The number density of the sample in
  Angstrom^-3.</LI>
  <LI> NumberOfWavelengthPoints - The number of wavelength points for which
  numerical integral is calculated (default: all points). </LI>
  <LI> ExpMethod - The method to calculate exponential function (Normal or Fast
  approximation). </LI>
  <LI> Thickness - The thickness of the sample holder in centimetres. </LI>
  <LI> ElementSize - The side dimension of an integration element cube in mm
  (default: 1). </LI>
  </UL>

  This algorithm uses the FlatPlateAbsorption algorithm to numerically calculate
  the
  correction for the sample itself, and then further modifies the correction to
  account
  for the vanadium windows at the front and rear of the sample holder, and the
  traversal
  of the holder material (aluminium) itself en route to the 90 degree bank.

  The further corrections calculated within this algorithm use an analytical
  approximation
  of the form exp(-mt/cos(theta)).

  @author Russell Taylor, Tessella plc
  @date 05/02/2010
*/
class MANTID_ALGORITHMS_DLL HRPDSlabCanAbsorption : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "HRPDSlabCanAbsorption"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates attenuation due to absorption and scattering in an HRPD "
           "'slab' can.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"AbsorptionCorrection"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\AbsorptionCorrections"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// runs the flat plate absorption
  API::MatrixWorkspace_sptr runFlatPlateAbsorption();
};

} // namespace Algorithms
} // namespace Mantid
