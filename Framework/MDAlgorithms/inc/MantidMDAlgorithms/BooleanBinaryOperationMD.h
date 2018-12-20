#ifndef MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMD_H_
#define MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** BooleanBinaryOperationMD : base class for boolean-type operations on
  MDHistoWorkspaces

  @date 2011-11-08

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
class DLLExport BooleanBinaryOperationMD : public BinaryOperationMD {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override;

  int version() const override;

protected:
  /// Return true if the algorithm can operate on a scalar.
  virtual bool acceptScalar() const { return true; }
  bool commutative() const override;
  void checkInputs() override;
  void execEvent() override;
  void execHistoScalar(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) override;

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override = 0;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_BOOLEANBINARYOPERATIONMD_H_ */
