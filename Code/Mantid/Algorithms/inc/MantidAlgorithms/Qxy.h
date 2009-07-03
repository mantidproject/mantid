#ifndef MANTID_ALGORITHMS_QXY_H_
#define MANTID_ALGORITHMS_QXY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm rebins a 2D workspace in units of wavelength into 2D Q.
    The results is stored in a 2D workspace and written to file in the FISH format.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The corrected data in units of wavelength. </LI>
    <LI> OutputWorkspace - The workspace in which to store data as x & y components of Q. </LI>
    <LI> OutputFilename  - The filename under which to store the 2d data </LI>
    <LI> MaxQxy          - The upper limit of the Qx-Qy grid (goes from -MaxQxy to +MaxQxy). </LI>
    <LI> DeltaQ          - The dimension of a Qx-Qy cell. </LI>
    </UL>
    
    @author Russell Taylor, Tessella plc
    @date 09/04/2009

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
class DLLExport Qxy : public API::Algorithm
{
public:
  /// (Empty) Constructor
  Qxy() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~Qxy() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Qxy"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  API::MatrixWorkspace_sptr setUpOutputWorkspace();
  void writeResult(API::MatrixWorkspace_const_sptr result);
  
  
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_QXY_H_*/
