#ifndef MANTID_CRYSTAL_NORMALISEVANADIUM_H_
#define MANTID_CRYSTAL_NORMALISEVANADIUM_H_
/*WIKI* 

  Following A.J.Schultz's anvred, the weight factors should be:
  sin^2(theta) / (lamda^4 * spec * eff * trans)
  where theta = scattering_angle/2
        lamda = wavelength (in angstroms?)
        spec  = incident spectrum correction
        eff   = pixel efficiency
        trans = absorption correction
  The quantity:
    sin^2(theta) / eff
  depends only on the pixel and can be pre-calculated
  for each pixel.  It could be saved in array pix_weight[].
  For now, pix_weight[] is calculated by the method:
  BuildPixWeights() and just holds the sin^2(theta) values.
  The wavelength dependent portion of the correction is saved in
  the array lamda_weight[].
  The time-of-flight is converted to wave length by multiplying
  by tof_to_lamda[id], then (int)STEPS_PER_ANGSTROM * lamda
  gives an index into the table lamda_weight[].
  The lamda_weight[] array contains values like:
      1/(lamda^power * spec(lamda))
  which are pre-calculated for each lamda.  These values are
  saved in the array lamda_weight[].  The optimal value to use
  for the power should be determined when a good incident spectrum
  has been determined.  Currently, power=3 when used with an
  incident spectrum and power=2.4 when used without an incident
  spectrum.
  The pixel efficiency and incident spectrum correction are NOT CURRENTLY USED.
  The absorption correction, trans, depends on both lamda and the pixel,
  Which is a fairly expensive calulation when done for each event.
--


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Crystal
{
  const double radtodeg_half = 180.0/M_PI/2.;
  /** Calculates anvred correction factors for attenuation due to absorption and scattering in a spherical sample.

      Properties:
      <UL>
      <LI> InputWorkspace  - The name of the input workspace. </LI>
      <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
      <LI> Wavelength - Value for interpolation of spectra
      </UL>

      @author Vickie Lynch SNS
      @date 10/09/2011

      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
      Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport NormaliseVanadium : public API::Algorithm
{
public:
  /// (Empty) Constructor
  NormaliseVanadium();
  /// Virtual destructor
  virtual ~NormaliseVanadium() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "NormaliseVanadium"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Crystal"; }

protected:
  /** A virtual function in which additional properties of an algorithm should be declared. 
   *  Called by init().
   */

  API::MatrixWorkspace_sptr m_inputWS;     ///< A pointer to the input workspace

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_NORMALISEVANADIUM_H_*/
