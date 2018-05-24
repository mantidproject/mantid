#ifndef MANTID_DATAHANDLING_DELETETABLEROWS_H_
#define MANTID_DATAHANDLING_DELETETABLEROWS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/**
Deletes a row from a TableWorkspace.

@author Roman Tolchenov, Tessella plc
@date 12/05/2011

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport DeleteTableRows : public API::Algorithm {
public:
  /// Default constructor
  DeleteTableRows() {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DeleteTableRows"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Deletes rows from a TableWorkspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CreateEmptyTableWorkspace"};
  }
  /// Category
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  /// Initialize the static base properties
  void init() override;
  /// Execute
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_DELETETABLEROWS_H_  */
