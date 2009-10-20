#ifndef MANTID_ALGORITHMS_Q1D_H_
#define MANTID_ALGORITHMS_Q1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Part of the 1D data reduction chain for SANS instruments.
    Takes a workspace in Wavelength as input, with many of the corrections
    applied, converts it to Q, rebins to the given parameters, corrects
    for solid angle (taking account of any bin or detector masking) and sums
    all spectra to produce a final single spectrum result workspace

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The (partly) corrected data in units of wavelength. </LI>
    <LI> InputForErrors  - The workspace containing the counts to use for the error calculation
                           Must also be in units of wavelength and have matching bins to the InputWorkspace. </LI>
    <LI> OutputWorkspace - The workspace in which to store the result histogram. </LI>
    <LI> OutputBinning   - The bin parameters to use for the final result. </LI>
    </UL>
 
    @author Russell Taylor, Tessella plc
    @date 16/10/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport Q1D : public API::Algorithm
{
public:
  /// (Empty) Constructor
  Q1D() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~Q1D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Q1D"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
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

#endif /*MANTID_ALGORITHMS_Q1D_H_*/
