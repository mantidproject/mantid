#ifndef MANTID_ALGORITHMS_DIVIDEVANADIUM_H_
#define MANTID_ALGORITHMS_DIVIDEVANADIUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Divides the workspace by Vanadium workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> VanadiumWorkspace - The name of the vanadium workspace. </LI>
    </UL>

    @author Vickie Lynch, ORNL
    @date 07/01/2011

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
class DLLExport DivideVanadium : public API::Algorithm
{
public:
  /// (Empty) Constructor
  DivideVanadium() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~DivideVanadium() {}
  /// Algorithm's name
  virtual const std::string name() const { return "DivideVanadium"; }
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

#endif /*MANTID_ALGORITHMS_DIVIDEVANADIUM_H_*/
