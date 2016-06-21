#ifndef MANTID_ALGORITHMS_SANSDIRECTBEAMSCALING_H_
#define MANTID_ALGORITHMS_SANSDIRECTBEAMSCALING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/**
    Computes the scaling factor to get reduced SANS data on an absolute scale.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> AttenuatorTransmission   - Attenuator transmission for empty direct
   beam. </LI>
    <LI> AttenuatorTransmissionError   - Uncertainty in attenuator transmission.
   </LI>
    <LI> BeamRadius   - Radius of the beam stop [m]. </LI>
    <LI> SourceApertureRadius   - Source aperture to be used if it is not found
   in the instrument parameters [m]. </LI>
    <LI> SampleApertureRadius   - Sample aperture to be used if it is not found
   in the instrument parameters [m]. </LI>
    <LI> BeamMonitor   - The UDET of the incident beam monitor.</LI>
    </UL>

    Output Properties:
    <UL>
    <LI> ScaleFactor   - Scale factor value and uncertainty [n/(monitor
   count)/(cm^2)/steradian].</LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SANSDirectBeamScaling : public API::Algorithm,
                                        public API::DeprecatedAlgorithm {
public:
  /// (Empty) Constructor
  SANSDirectBeamScaling() : API::Algorithm() { deprecatedDate("2014-06-12"); }
  /// Algorithm's name
  const std::string name() const override { return "SANSDirectBeamScaling"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Computes the scaling factor to get reduced SANS data on an "
           "absolute scale.";
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

#endif /*MANTID_ALGORITHMS_SANSDIRECTBEAMSCALING_H_*/
