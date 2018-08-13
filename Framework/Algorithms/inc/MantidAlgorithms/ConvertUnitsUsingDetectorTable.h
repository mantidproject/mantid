#ifndef MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_
#define MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

/** ConvertUnitsUsingDetectorTable : Converts the units in which a workspace is
  represented, this variant of ConvertUnits uses a supplied table of geometry
  values
  rather than those given by the instrument geometry.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ConvertUnitsUsingDetectorTable
    : public ConvertUnits,
      public API::DeprecatedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  const std::string workspaceMethodName() const override { return ""; }
  const std::string workspaceMethodInputProperty() const override { return ""; }

private:
  void init() override;

  void storeEModeOnWorkspace(API::MatrixWorkspace_sptr outputWS) override;

  /// Convert the workspace units using TOF as an intermediate step in the
  /// conversion
  API::MatrixWorkspace_sptr
  convertViaTOF(Kernel::Unit_const_sptr fromUnit,
                API::MatrixWorkspace_const_sptr inputWS) override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_ */
