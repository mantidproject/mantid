#ifndef MANTID_MDALGORITHMS_XORMD_H_
#define MANTID_MDALGORITHMS_XORMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BooleanBinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** XorMD : boolean operation on MDHistoWorkspaces

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
class DLLExport XorMD : public BooleanBinaryOperationMD {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AndMD", "OrMD", "NotMD"};
  }

private:
  void execHistoHisto(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_XORMD_H_ */
