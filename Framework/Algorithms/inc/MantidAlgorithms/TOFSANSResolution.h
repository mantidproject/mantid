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
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
    Computes the resolution on TOF SANS data
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
class MANTID_ALGORITHMS_DLL TOFSANSResolution : public API::Algorithm {
public:
  /// Defatult constructor
  TOFSANSResolution();
  /// Default destructor
  ~TOFSANSResolution() override = default;
  /// Algorithm's name
  const std::string name() const override { return "TOFSANSResolution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculate the Q resolution for TOF SANS data."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"TOFSANSResolutionByPixel"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Return the TOF resolution for a particular wavelength
  virtual double getTOFResolution(double wl);
  /// Return the effective pixel size in the x-direction
  virtual double getEffectiveXPixelSize();
  /// Return the effective pixel size in the y-direction
  virtual double getEffectiveYPixelSize();
  /// Wavelength resolution (constant for all wavelengths)
  double m_wl_resolution;
};

} // namespace Algorithms
} // namespace Mantid
