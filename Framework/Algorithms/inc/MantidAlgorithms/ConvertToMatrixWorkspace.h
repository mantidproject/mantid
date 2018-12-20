#ifndef MANTID_ALGORITHMS_CONVERTTOMATRIXWORKSPACE_H_
#define MANTID_ALGORITHMS_CONVERTTOMATRIXWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Creates a copy of the matrix workspace representation of the input
 workspace. At the moment, this
 is only available for MatrixWorkspaces and EventWorkspaces.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input workspace. </LI>
 <LI> OutputWorkspace - The name of the output workspace. </LI>
 </UL>

 @author Stuart Campbell, ORNL
 @date 10/12/2010

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
class DLLExport ConvertToMatrixWorkspace : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertToMatrixWorkspace"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts an EventWorkspace into a Workspace2D, using the input "
           "workspace's current X bin values.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToEventWorkspace", "Rebin"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Events"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTTOMATRIXWORKSPACE_H_*/
