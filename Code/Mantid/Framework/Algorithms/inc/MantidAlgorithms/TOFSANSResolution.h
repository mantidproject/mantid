#ifndef MANTID_ALGORITHMS_TOFSANSRESOLUTION_H_
#define MANTID_ALGORITHMS_TOFSANSRESOLUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

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
class DLLExport TOFSANSResolution : public API::Algorithm {
public:
  /// (Empty) Constructor
  TOFSANSResolution() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~TOFSANSResolution() {}
  /// Algorithm's name
  virtual const std::string name() const { return "TOFSANSResolution"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the Q resolution for TOF SANS data.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Return the TOF resolution for a particular wavelength
  virtual double getTOFResolution(double wl);
  /// Wavelength resolution (constant for all wavelengths)
  double wl_resolution;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_TOFSANSRESOLUTION_H_*/
