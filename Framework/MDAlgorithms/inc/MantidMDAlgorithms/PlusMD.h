#ifndef MANTID_MDALGORITHMS_PLUSMD_H_
#define MANTID_MDALGORITHMS_PLUSMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** Sum two MDWorkspaces together.

  @date 2011-08-12

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport PlusMD : public BinaryOperationMD {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "PlusMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sum two MDHistoWorkspaces or merges two MDEventWorkspaces together "
           "by combining their events together in one workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MinusMD", "MultiplyMD", "DivideMD", "PowerMD"};
  }

private:
  /// Is the operation commutative?
  bool commutative() const override;

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs() override;

  /// Run the algorithm with an MDEventWorkspace as output
  void execEvent() override;

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override;

  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void execHistoScalar(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) override;

  template <typename MDE, size_t nd>
  void doPlus(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Workspace into which stuff will get added
  Mantid::API::IMDEventWorkspace_sptr iws1;
  /// Workspace that will be added into ws1
  Mantid::API::IMDEventWorkspace_sptr iws2;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_PLUSMD_H_ */
