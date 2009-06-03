#ifndef MANTID_ALGORITHMS_CROPWORKSPACE_H_
#define MANTID_ALGORITHMS_CROPWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{
/** Extracts a 'block' from a workspace and places it in a new workspace.
    (Or, to look at it another way, lops bins or spectra off a workspace).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input. Must have common bins. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> XMin - The X value to start the cropped workspace at (default 0)</LI>
    <LI> XMax - The X value to end the cropped workspace at (default max)</LI>
    <LI> StartSpectrum - The workspace index number to start the cropped workspace from (default 0)</LI>
    <LI> EndSpectrum - The workspace index number to end the cropped workspace at (default max)</LI>
    </UL>

    If an X value given does not exactly match a bin boundary, then the closest bin boundary
    within the range will be used.
    Note that if none of the optional properties are given, then the output workspace will be
    a copy of the input one.

    @author Russell Taylor, Tessella Support Services plc
    @date 15/10/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport CropWorkspace : public API::Algorithm
{
public:
  CropWorkspace();
  virtual ~CropWorkspace();
  /// Algorithm's name
  virtual const std::string name() const { return "CropWorkspace"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void checkProperties();
  void getXMin();
  void getXMax();

  /// The input workspace
  DataObjects::Workspace2D_const_sptr m_inputWorkspace;
  /// The bin index to start the cropped workspace from
  int m_minX;
  /// The bin index to end the cropped workspace at
  int m_maxX;
  /// The spectrum index to start the cropped workspace from
  int m_minSpec;
  /// The spectrum index to end the cropped workspace at
  int m_maxSpec;
  
  ///a flag int value to indicate that the value wasn't set by users
  static const int unSetInt = INT_MAX-15;

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CROPWORKSPACE_H_*/
