#ifndef MANTID_ALGORITHMS_CLONEWORKSPACE_H_
#define MANTID_ALGORITHMS_CLONEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Creates a copy of the input workspace. At the moment, this is only available for MatrixWorkspaces, though
    it should be perfectly possible to extend this to include TableWorkspaces if that is ever required.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 09/12/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport CloneWorkspace : public API::Algorithm
{
public:
  /// (Empty) Constructor
  CloneWorkspace() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~CloneWorkspace() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CloneWorkspace"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CLONEWORKSPACE_H_*/
