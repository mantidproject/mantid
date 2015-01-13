#ifndef MANTID_MDALGORITHMS_MERGEMD_H_
#define MANTID_MDALGORITHMS_MERGEMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Merge several MDWorkspaces into one.

  @date 2012-01-17

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
class DLLExport MergeMD
    : public Mantid::MDEvents::BoxControllerSettingsAlgorithm {
public:
  MergeMD();
  virtual ~MergeMD();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Merge several MDWorkspaces into one.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  void createOutputWorkspace(std::vector<std::string> &inputs);

  template <typename MDE, size_t nd>
  void doPlus(typename Mantid::MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Vector of input MDWorkspaces
  std::vector<Mantid::API::IMDEventWorkspace_sptr> m_workspaces;

  /// Output MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr out;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MERGEMD_H_ */
