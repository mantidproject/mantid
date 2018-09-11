#ifndef MANTID_MDALGORITHMS_TRANSPOSEMD_H_
#define MANTID_MDALGORITHMS_TRANSPOSEMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/MDAxisValidator.h"
#include "MantidMDAlgorithms/DllConfig.h"
namespace Mantid {
namespace MDAlgorithms {

/** TransposeMD : Transpose an MDWorkspace. Allows dimensions to be collapsed
  down.

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
class MANTID_MDALGORITHMS_DLL TransposeMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Transpose3D", "Transpose"};
  }
  const std::string category() const override;
  const std::string summary() const override;

  const std::string alias() const override { return "PermuteMD"; }

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_TRANSPOSEMD_H_ */
