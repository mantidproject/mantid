#ifndef MANTID_ALGORITHMS_EQSANSRESOLUTION_H_
#define MANTID_ALGORITHMS_EQSANSRESOLUTION_H_

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
class DLLExport EQSANSResolution : public Algorithms::TOFSANSResolution {
public:
  /// (Empty) Constructor
  EQSANSResolution() : Algorithms::TOFSANSResolution() {}
  /// Virtual destructor
  virtual ~EQSANSResolution() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSResolution"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the Q resolution for EQSANS data.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  // void init();
  /// Execution code
  // void exec();
  /// Return the TOF resolution for a particular wavelength
  virtual double getTOFResolution(double wl);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSRESOLUTION_H_*/
