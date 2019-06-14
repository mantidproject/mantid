// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SANSSOLIDANGLECORRECTION_H_
#define MANTID_ALGORITHMS_SANSSOLIDANGLECORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Performs a solid angle correction on a 2D SANS data set to correct
    for the absence of curvature of the detector.

    Note: one could use SolidAngle to perform this calculation. Solid Angle
    returns the solid angle of each detector pixel, but that approach requires
   more un-necessary calculations.

    In the contrary, this algorithm performs analytical correction as follows:
      Omega(theta) = Omega(0) cos^3(theta)
      where Omega is the solid angle.
     so we simply apply the
   cos^3(theta).

    Brulet et al, J. Appl. Cryst. (2007) 40, 165-177.
    See equation 22.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> OutputWorkspace   - The workspace in which to store the result
   histogram. </LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SANSSolidAngleCorrection : public API::Algorithm {
public:
  SANSSolidAngleCorrection();

  /// Algorithm's name
  const std::string name() const override { return "SANSSolidAngleCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs solid angle correction on SANS 2D data.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SolidAngle", "SANSBeamFluxCorrection"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager;"
           "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execEvent();
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSSOLIDANGLECORRECTION_H_*/
