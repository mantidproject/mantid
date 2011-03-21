#ifndef MANTID_ALGORITHMS_SANSDIRECTBEAMSCALING_H_
#define MANTID_ALGORITHMS_SANSDIRECTBEAMSCALING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**
    Computes the scaling factor to get reduced SANS data on an absolute scale.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> AttenuatorTransmission   - Attenuator transmission for empty direct beam. </LI>
    <LI> AttenuatorTransmissionError   - Uncertainty in attenuator transmission. </LI>
    <LI> BeamRadius   - Radius of the beam stop [m]. </LI>
    <LI> SourceApertureRadius   - Source aperture to be used if it is not found in the instrument parameters [m]. </LI>
    <LI> SampleApertureRadius   - Sample aperture to be used if it is not found in the instrument parameters [m]. </LI>
    <LI> BeamMonitor   - The UDET of the incident beam monitor.</LI>
    </UL>

    Output Properties:
    <UL>
    <LI> ScaleFactor   - Scale factor value and uncertainty [n/(monitor count)/(cm^2)/steradian].</LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SANSDirectBeamScaling : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SANSDirectBeamScaling() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SANSDirectBeamScaling() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SANSDirectBeamScaling"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSDIRECTBEAMSCALING_H_*/
