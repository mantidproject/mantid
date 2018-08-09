#ifndef MANTID_ALGORITHMS_RECORDPYTHONSCRIPT_H_
#define MANTID_ALGORITHMS_RECORDPYTHONSCRIPT_H_

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAlgorithms/GeneratePythonScript.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** RecordPythonScript : TODO: DESCRIPTION

  An Algorithm to generate a Python script file to reproduce the history of a
  workspace.

  Properties:
  <ul>
  <li>Filename - the name of the file to write to. </li>
  <li>InputWorkspace - the workspace name who's history is to be saved.</li>
  </ul>

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
class DLLExport RecordPythonScript : public Algorithms::GeneratePythonScript,
                                     public API::AlgorithmObserver {
public:
  RecordPythonScript();
  /// Algorithm's name for identification
  const std::string name() const override { return "RecordPythonScript"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An Algorithm to generate a Python script file to reproduce the "
           "history of a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"GenerateIPythonNotebook", "GeneratePythonScript"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Python"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /** Handler of the start notifications. Must be overriden in inherited
  classes.
  @param alg :: Shared Pointer to the algorithm sending the notification.
  */
  void startingHandle(API::IAlgorithm_sptr alg) override;
  /// buffer for the script
  std::string m_generatedScript;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RECORDPYTHONSCRIPT_H_ */
