#ifndef MANTID_ALGORITHMS_SANSSOLIDANGLECORRECTION_H_
#define MANTID_ALGORITHMS_SANSSOLIDANGLECORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Performs a solid angle correction on a 2D SANS data set to correct
    for the absence of curvature of the detector.

    Note: one could use SolidAngle to perform this calculation. Solid Angle
    returns the solid angle of each detector pixel. The correction is then
    given by:
      Omega(theta) = Omega(0) cos^3(theta)
      where Omega is the solid angle.
    This approach requires more un-necessary calculations so we simply apply the
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
  /// (Empty) Constructor
  SANSSolidAngleCorrection() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SANSSolidAngleCorrection() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SANSSolidAngleCorrection"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Performs solid angle correction on SANS 2D data.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Workflow\\SANS\\UsesPropertyManager;"
           "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void execEvent();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSSOLIDANGLECORRECTION_H_*/
