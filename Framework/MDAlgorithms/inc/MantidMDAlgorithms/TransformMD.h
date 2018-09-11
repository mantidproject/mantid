#ifndef MANTID_MDALGORITHMS_TRANSFORMMD_H_
#define MANTID_MDALGORITHMS_TRANSFORMMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Scale and/or offset the coordinates of a MDWorkspace

  @date 2012-01-18

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport TransformMD : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Scale and/or offset the coordinates of a MDWorkspace";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"InvertMDDim"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void reverse(signal_t *array, size_t arrayLength);
  Mantid::DataObjects::MDHistoWorkspace_sptr
  transposeMD(Mantid::DataObjects::MDHistoWorkspace_sptr &toTranspose,
              const std::vector<int> &axes);
  template <typename MDE, size_t nd>
  void
  doTransform(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  std::vector<double> m_scaling;
  std::vector<double> m_offset;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_TRANSFORMMD_H_ */
