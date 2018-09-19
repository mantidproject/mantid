#ifndef MANTID_MDALGORITHMS_CREATEMDHISTOWORKSPACE_H_
#define MANTID_MDALGORITHMS_CREATEMDHISTOWORKSPACE_H_

#include "MantidMDAlgorithms/ImportMDHistoWorkspaceBase.h"

namespace Mantid {
namespace MDAlgorithms {

/** CreateMDHistoWorkspace

  @date 2012-06-21

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
class DLLExport CreateMDHistoWorkspace
    : public MDAlgorithms::ImportMDHistoWorkspaceBase {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates an MDHistoWorkspace from supplied lists of signal and "
           "error values.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD", "CreateMDWorkspace"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CREATEMDHISTOWORKSPACE_H_ */
