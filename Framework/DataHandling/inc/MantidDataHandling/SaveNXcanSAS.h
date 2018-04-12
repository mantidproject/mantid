#ifndef MANTID_DATAHANDLING_SAVENXCANSAS_H_
#define MANTID_DATAHANDLING_SAVENXCANSAS_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/** SaveNXcanSAS : Saves a reduced workspace in the NXcanSAS format. Currently
 * only MatrixWorkspaces resulting from 1D and 2D reductions are supported.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL SaveNXcanSAS : public API::Algorithm {
public:
  /// Constructor
  SaveNXcanSAS();
  /// Virtual dtor
  ~SaveNXcanSAS() override {}
  const std::string name() const override { return "SaveNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes a MatrixWorkspace to a file in the NXcanSAS format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveCanSAS1D", "LoadNXcanSAS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

std::string MANTID_DATAHANDLING_DLL
makeCanSASRelaxedName(const std::string &input);

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVENXCANSAS_H_ */
