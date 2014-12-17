#ifndef MANTID_MDALGORITHMS_MINUSMD_H_
#define MANTID_MDALGORITHMS_MINUSMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** MinusMD : minus operation for MDWorkspaces

  @date 2011-11-07

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
class DLLExport MinusMD : public BinaryOperationMD {
public:
  MinusMD();
  virtual ~MinusMD();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Subtract two MDWorkspaces.";
  }

  virtual int version() const;

private:
  /// Is the operation commutative?
  bool commutative() const;

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs();

  template <typename MDE, size_t nd>
  void doMinus(typename Mantid::MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Run the algorithm with an MDEventWorkspace as output
  void execEvent();

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out,
                      Mantid::MDEvents::MDHistoWorkspace_const_sptr operand);

  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void
  execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out,
                  Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MINUSMD_H_ */
