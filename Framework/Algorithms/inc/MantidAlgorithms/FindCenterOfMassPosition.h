#ifndef MANTID_ALGORITHMS_FINDCENTEROFMASSPOSITION_H_
#define MANTID_ALGORITHMS_FINDCENTEROFMASSPOSITION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
    Part of data reduction for SANS. Find the center of mass of the 2D detector
   counts.
    Assumes that all monitors are at the top of the spectra list, and that the
   spectra
    run through columns (Y is that fast running index).

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> Output            - The name of the table workspace containing the
   center of mass position.
                             If none is provided, an ArrayProperty named
   CenterOfMass will contain the result.</LI>
    <LI> NPixelX           - Number of detector pixels in the X direction.
   Default: 192</LI>
    <LI> NPixelY           - Number of detector pixels in the Y direction.
   default: 192</LI>
    <LI> DirectBeam        - If true, a direct beam calculation will be
   performed. Otherwise, the center of mass
                             of the scattering data will be computed by
   excluding the beam area. Default: true</LI>
    <LI> BeamRadius        - Radius of the beam area, in pixels, used the
   exclude the beam when calculating
                             the center of mass of the scattering pattern.
   Default: 20 pixels</LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FindCenterOfMassPosition : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "FindCenterOfMassPosition"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Finds the beam center in a 2D SANS data set.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FINDCENTEROFMASSPOSITION_H_*/
