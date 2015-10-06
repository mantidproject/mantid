#ifndef MANTID_ALGORITHMS_NORMALISETOUNITY_H_
#define MANTID_ALGORITHMS_NORMALISETOUNITY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Takes a workspace as input and normalises it to 1.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. Must be a
   histogram. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> RangeLower - The X value to integrate from (default 0)</LI>
    <LI> RangeUpper - The X value to integrate to (default max)</LI>
    <LI> StartWorkspaceIndex - Workspace index number to integrate from (default
   0)</LI>
    <LI> EndWorkspaceIndex - Workspace index number to integrate to (default
   max)</LI>
    <LI> IncludePartialBins - If true then partial bins from the beginning and
   end of the input range are also included in the integration (default
   false)</LI>
    <LI> IncludeMonitors - Whether to include monitor spectra in the sum
   (default yes)
    </UL>

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport NormaliseToUnity : public API::Algorithm {
public:
  /// Default constructor
  NormaliseToUnity() : API::Algorithm(){};
  /// Destructor
  virtual ~NormaliseToUnity(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "NormaliseToUnity"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "NormaliseToUnity takes a 2D workspace or an EventWorkspace as "
           "input and normalises it to 1. Optionally, the range summed can be "
           "restricted in either dimension.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "CorrectionFunctions\\NormalisationCorrections";
  }

private:
  // Overridden Algorithm methods
  void init();
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_NORMALISETOUNITY_H_*/
