#ifndef MANTID_ALGORITHMS_APPLYTRANSMISSIONCORRECTION_H_
#define MANTID_ALGORITHMS_APPLYTRANSMISSIONCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**

    Apply angle-dependent transmission correction from zero-angle transmission measurement.

    The CalculateTransmission algorithm computes the transmission at theta=0. For shorter
    sample-detector distances, the transmission correction is a function of the transmission
    at theta=0 and the scattering angle:

    T(theta) = T^[(1+sec(2theta))/2]

    The zero-angle transmission can either be specified in a workspace with wavelength binning
    consistent with the input workspace, or be specified as a constant with error.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> OutputWorkspace   - The workspace in which to store the result histogram. </LI>
    <LI> TransmissionWorkspace - The workspace containing the zero-angle transmission </LI>
    <LI> TransmissionValue - Transmission value to apply to all wavelengths. If specified, TransmissionWorkspace will not be used </LI>
    <LI> TransmissionError - The error on the transmission value (default 0.0) </LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ApplyTransmissionCorrection : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ApplyTransmissionCorrection() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ApplyTransmissionCorrection() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ApplyTransmissionCorrection"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_APPLYTRANSMISSIONCORRECTION_H_*/
