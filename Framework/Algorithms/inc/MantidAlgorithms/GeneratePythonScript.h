#ifndef MANTID_ALGORITHMS_GENERATEPYTHONSCRIPT_H_
#define MANTID_ALGORITHMS_GENERATEPYTHONSCRIPT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/SerialAlgorithm.h"

namespace Mantid {
namespace Algorithms {

/** GeneratePythonScript : TODO: DESCRIPTION

  An Algorithm to generate a Python script file to reproduce the history of a
  workspace.

  Properties:
  <ul>
  <li>Filename - the name of the file to write to. </li>
  <li>InputWorkspace - the workspace name who's history is to be saved.</li>
  </ul>

  @author Peter G Parker, ISIS, RAL
  @date 2011-09-13

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport GeneratePythonScript : public API::SerialAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "GeneratePythonScript"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An Algorithm to generate a Python script file to reproduce the "
           "history of a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"RecordPythonScript", "GenerateIPythonNotebook"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Python"; }

protected:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GENERATEPYTHONSCRIPT_H_ */
