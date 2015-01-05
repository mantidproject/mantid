#ifndef MANTID_CRYSTAL_NORMALISEVANADIUM_H_
#define MANTID_CRYSTAL_NORMALISEVANADIUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Crystal {
const double radtodeg_half = 180.0 / M_PI / 2.;
/** Calculates anvred correction factors for attenuation due to absorption and
   scattering in a spherical sample.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> Wavelength - Value for interpolation of spectra
    </UL>

    @author Vickie Lynch SNS
    @date 10/09/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport NormaliseVanadium : public API::Algorithm {
public:
  /// (Empty) Constructor
  NormaliseVanadium();
  /// Virtual destructor
  virtual ~NormaliseVanadium() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "NormaliseVanadium"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Normalises all spectra to a specified wavelength.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "Crystal;CorrectionFunctions\\NormalisationCorrections";
  }

protected:
  /** A virtual function in which additional properties of an algorithm should
   * be declared.
   *  Called by init().
   */

  API::MatrixWorkspace_sptr m_inputWS; ///< A pointer to the input workspace

protected:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_NORMALISEVANADIUM_H_*/
