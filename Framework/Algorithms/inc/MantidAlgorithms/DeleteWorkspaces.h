#ifndef MANTID_ALGORITHMS_DELETEWORKSPACES_H_
#define MANTID_ALGORITHMS_DELETEWORKSPACES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
  A simple algorithm to remove multiple workspaces from the ADS.

  @author Nick Draper, Tessella plc
  @date 2017-02-17

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport DeleteWorkspaces : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "DeleteWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes a list of workspaces from memory.";
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"DeleteWorkspace"};
  }

private:
  /// Overridden init
  void init() override;
  /// Overridden exec
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHMS_DELETEWORKSPACES_H_
