#ifndef MANTID_MDALGORITHMS_CUTMD_H_
#define MANTID_MDALGORITHMS_CUTMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidKernel/System.h"
#include <MantidAPI/IMDWorkspace.h>

namespace Mantid {
namespace MDAlgorithms {

std::vector<std::string>
    DLLExport findOriginalQUnits(Mantid::API::IMDWorkspace_const_sptr inws,
                                 Mantid::Kernel::Logger &logger);

/** CutMD : Slices multidimensional workspaces.

  @date 2015-03-20

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
class DLLExport CutMD : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override { return "CutMD"; }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SliceMDHisto", "ProjectMD", "SliceMD", "BinMD"};
  }
  const std::string summary() const override {
    return "Slices multidimensional workspaces using input projection "
           "information and binning limits.";
  }
  const std::string category() const override {
    return "MDAlgorithms\\Slicing";
  }

  void init() override;
  void exec() override;

  static const std::string InvAngstromSymbol;
  static const std::string RLUSymbol;
  static const std::string AutoMethod;
  static const std::string RLUMethod;
  static const std::string InvAngstromMethod;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CUTMD_H_ */
