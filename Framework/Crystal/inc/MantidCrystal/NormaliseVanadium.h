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
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Crystal {
const double radtodeg_half = 180.0 / M_PI / 2.;
/** Calculates anvred correction factors for attenuation due to absorption and
   scattering in a spherical sample.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> Wavelength - Value for interpolation of spectra
    </UL>

    @author Vickie Lynch SNS
    @date 10/09/2011
*/
class MANTID_CRYSTAL_DLL NormaliseVanadium : public API::Algorithm {
public:
  /// (Empty) Constructor
  NormaliseVanadium();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "NormaliseVanadium"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Normalises all spectra to a specified wavelength."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Crystal\\Corrections;CorrectionFunctions\\NormalisationCorrections";
  }

protected:
  /** A virtual function in which additional properties of an algorithm should
   * be declared.
   *  Called by init().
   */

  API::MatrixWorkspace_sptr m_inputWS; ///< A pointer to the input workspace

protected:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
