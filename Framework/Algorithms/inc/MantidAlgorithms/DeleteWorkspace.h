#ifndef MANTID_ALGORITHMS_DELETEWORKSPACE_H_
#define MANTID_ALGORITHMS_DELETEWORKSPACE_H_

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/**
  A simple algorithm to remove a workspace from the ADS. Basically so that it is
  logged
  and can be recreated from a history record

  @author Martyn Gigg, Tessella plc
  @date 2011-01-24

  Copyright &copy; 20011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport DeleteWorkspace : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "DeleteWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes a workspace from memory.";
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"DeleteWorkspaces"};
  }

private:
  /// Overridden init
  void init() override;
  /// Overridden exec
  void exec() override;

  const std::string workspaceMethodName() const override { return "delete"; }
  const std::string workspaceMethodInputProperty() const override {
    return "Workspace";
  }
};

} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHMS_DELETEWORKSPACE_H_
