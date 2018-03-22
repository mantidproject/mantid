#ifndef MANTID_MDALGORITHMS_CLONEMDWORKSPACE_H_
#define MANTID_MDALGORITHMS_CLONEMDWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm to clone a MDEventWorkspace to a new one.
 * Can also handle file-backed MDEventWorkspace's

  @author Janik Zikovsky
  @date 2011-08-15

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
class DLLExport CloneMDWorkspace : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CloneMDWorkspace"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Clones (copies) an existing MDEventWorkspace or MDHistoWorkspace "
           "into a new one.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(MDAlgorithms\Utility\Workspaces;MDAlgorithms\Creation)";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  template <typename MDE, size_t nd>
  void doClone(const typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CLONEMDWORKSPACE_H_ */
