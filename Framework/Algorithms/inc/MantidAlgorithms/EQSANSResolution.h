// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TOFSANSResolution.h"

namespace Mantid {
namespace Algorithms {
/**

    Computes the resolution on EQSANS data
    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> OutputWorkspace   - The workspace in which to store the result
   histogram. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> MinWavelength     - Low wavelength cut. </LI>
    <LI> MaxWavelength     - Low wavelength cut. </LI>
    <LI> PixelSizeX        - Pixel size in X (mm). </LI>
    <LI> PixelSizeY        - Pixel size in Y (mm). </LI>
    <LI> SampleApertureRadius - Sample aperture radius (mm). </LI>
    <LI> SourceApertureRadius - Source aperture radius (mm). </LI>
    <LI> DeltaT            - TOF spread (microsec). </LI>
    <LI> </LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_ALGORITHMS_DLL EQSANSResolution : public Algorithms::TOFSANSResolution {
public:
  /// Algorithm's name
  const std::string name() const override { return "EQSANSResolution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculate the Q resolution for EQSANS data."; }
  const std::vector<std::string> seeAlso() const override { return {"ReactorSANSResolution"}; }

private:
  /// Initialisation code
  // void init();
  /// Execution code
  // void exec();
  /// Return the TOF resolution for a particular wavelength
  double getTOFResolution(double wl) override;
  /// Return the effective pixel size in the x-direction
  double getEffectiveXPixelSize() override { return 0.011; };
  /// Return the effective pixel size in the y-direction
  double getEffectiveYPixelSize() override { return 0.007; };
};

} // namespace Algorithms
} // namespace Mantid
