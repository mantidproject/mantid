#ifndef MANTID_ALGORITHMS_RECORDPYTHONSCRIPT_H_
#define MANTID_ALGORITHMS_RECORDPYTHONSCRIPT_H_

#include "MantidKernel/System.h"
#include "MantidAlgorithms/GeneratePythonScript.h"
#include "MantidAPI/AlgorithmObserver.h"

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
  virtual ~RecordPythonScript() {}

  /// Algorithm's name for identification
  virtual const std::string name() const { return "RecordPythonScript"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "An Algorithm to generate a Python script file to reproduce the "
           "history of a workspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Utility;PythonAlgorithms";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /** Handler of the start notifications. Must be overriden in inherited
  classes.
  @param alg :: Shared Pointer to the algorithm sending the notification.
  */
  void startingHandle(API::IAlgorithm_sptr alg);
  /// buffer for the script
  std::string m_generatedScript;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RECORDPYTHONSCRIPT_H_ */
