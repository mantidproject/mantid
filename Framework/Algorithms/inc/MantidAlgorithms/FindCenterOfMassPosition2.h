#ifndef MANTID_ALGORITHMS_FINDCENTEROFMASSPOSITION2_H_
#define MANTID_ALGORITHMS_FINDCENTEROFMASSPOSITION2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
    Part of data reduction for SANS. Find the center of mass of the 2D detector
   counts.
    Output is in meters.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> Output            - The name of the table workspace containing the
   center of mass position.
                             If none is provided, an ArrayProperty named
   CenterOfMass will contain the result.</LI>
    <LI> CenterX           - Estimate for the beam center in X [m]. Default:
   0</LI>
    <LI> CenterY           - Estimate for the beam center in Y [m]. Default:
   0</LI>
    <LI> Tolerance         - Tolerance on the center of mass position between
   each iteration [m]. Default: 0.00125</LI>
    <LI> DirectBeam        - If true, a direct beam calculation will be
   performed. Otherwise, the center of mass
                             of the scattering data will be computed by
   excluding the beam area. Default: true</LI>
    <LI> BeamRadius        - Radius of the beam area, in meters, used the
   exclude the beam when calculating
                             the center of mass of the scattering pattern.
   Default: 20 pixels</LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FindCenterOfMassPosition2 : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "FindCenterOfMassPosition"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Finds the beam center in a 2D SANS data set.";
  }

  /// Algorithm's version
  int version() const override { return (2); }

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

#endif /*MANTID_ALGORITHMS_FINDCENTEROFMASSPOSITION2_H_*/
