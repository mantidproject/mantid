#ifndef MANTID_MDALGORITHMS_MASKMD_H_
#define MANTID_MDALGORITHMS_MASKMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

std::vector<std::string> MANTID_MDALGORITHMS_DLL
parseDimensionNames(const std::string &names_string);

/** MaskMD : Mask an MDWorkspace. Can provide complex masking shapes over an
  exisitng MDWorkspace. Operates on a MDWorkspace in-situ.

  @date 2012-03-01

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

  File change history is stored at:
  <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_MDALGORITHMS_DLL MaskMD : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Mask an MDWorkspace in-situ marking specified boxes as masked";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MaskDetectors"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MASKMD_H_ */