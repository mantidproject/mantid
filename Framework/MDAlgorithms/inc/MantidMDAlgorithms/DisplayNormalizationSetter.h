#ifndef _MANTID_MDALGORITHMS_DISPLAYNORMALIZATION_SETTER_H
#define _MANTID_MDALGORITHMS_DISPLAYNORMALIZATION_SETTER_H
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {
/** DisplayNormalizationSetter: Sets the displaynormalization on a
    workspace based on several parameters such as workspace-type,
energy-transfer-mode
    and if we are dealing with Q3D.

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
class DLLExport DisplayNormalizationSetter {
public:
  void operator()(Mantid::API::IMDWorkspace_sptr mdWorkspace,
                  const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace,
                  bool isQ = false,
                  const Mantid::Kernel::DeltaEMode::Type &mode =
                      Mantid::Kernel::DeltaEMode::Elastic);

private:
  void setNormalizationMDEvent(
      Mantid::API::IMDWorkspace_sptr mdWorkspace,
      const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace,
      bool isQ = false,
      const Mantid::Kernel::DeltaEMode::Type &mode =
          Mantid::Kernel::DeltaEMode::Elastic);

  void applyNormalizationMDEvent(
      Mantid::API::IMDWorkspace_sptr mdWorkspace,
      Mantid::API::MDNormalization displayNormalization,
      Mantid::API::MDNormalization displayNormalizationHisto);
};
} // namespace MDAlgorithms
} // namespace Mantid
#endif