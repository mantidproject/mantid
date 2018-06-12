#ifndef MANTID_MDALGORITHMS_CONVERTMDHISTOTOMATRIXWORKSPACE_H_
#define MANTID_MDALGORITHMS_CONVERTMDHISTOTOMATRIXWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace API {
class IMDHistoWorkspace;
}

namespace MDAlgorithms {
/** Creates a single spectrum Workspace2D with X,Y, and E copied from an first
 non-integrated dimension of a IMDHistoWorkspace.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input IMDHistoWorkspace.. </LI>
 <LI> OutputWorkspace - The name of the output matrix workspace. </LI>
 <LI> Normalization -   Signal normalization method: NoNormalization,
 VolumeNormalization, or NumEventsNormalization</LI>
 </UL>

 @author Roman Tolchenov, Tessella plc
 @date 17/04/2012

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
class DLLExport ConvertMDHistoToMatrixWorkspace : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "ConvertMDHistoToMatrixWorkspace";
  };

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts if it can a IMDHistoWorkspace to a Workspace2D.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD", "CreateMDHistoWorkspace",
            "ConvertTableToMatrixWorkspace", "MDHistoToWorkspace2D"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Utility\\Workspaces;MDAlgorithms\\Transforms";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Make MatrixWorkspace with 1 spectrum
  void make1DWorkspace();
  /// Make 2D MatrixWorkspace
  void make2DWorkspace();
  /// Calculate the stride for a dimension
  size_t calcStride(const API::IMDHistoWorkspace &workspace, size_t dim) const;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_CONVERTMDHISTOTOMATRIXWORKSPACE_H_*/
