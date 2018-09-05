#ifndef MANTID_DATAHANDLING_SaveILLCosmosAscii_H_
#define MANTID_DATAHANDLING_SaveILLCosmosAscii_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid {
namespace DataHandling {
/**
This algorithm is deprecated. Please use SaveReflectometryAscii instead.

Saves a file in ILL Cosmos format  from a 2D workspace
(Workspace2D class). SaveILLCosmosAscii is an algorithm but inherits from the
AsciiPointBase class which provides the main implementation for the init() &
exec() methods.
Output is tab delimited Ascii point data with dq/q and extra header information.

Copyright &copy; 2007-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SaveILLCosmosAscii : public DataHandling::AsciiPointBase,
                                     public API::DeprecatedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveILLCosmosAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file of format mft.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveAscii"};
  }
  SaveILLCosmosAscii() {
    this->useAlgorithm("SaveReflectometryAscii");
    this->deprecatedDate("2018-06-29");
  }

private:
  /// Return the file extension this algorthm should output.
  std::string ext() override { return ".mft"; }
  /// extra properties specifically for this
  void extraProps() override;
  /// write any extra information required
  void extraHeaders(std::ofstream &file) override;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SaveILLCosmosAscii_H_  */
