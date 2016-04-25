#ifndef MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H

#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataPreprocessorAlgorithm

DataPreprocessorAlgorithm is an class which defines a pre-processor algorithm.
A pre-processor algorithm is an algorithm that will pre-process a column in a
Data Processor UI.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DataPreprocessorAlgorithm : public DataProcessorAlgorithm {
public:
  /** Constructor
  * @param name : The name of the pre-processing algorithm
  * @param prefix : The list of prefixes that will be used for the output
  * workspaces' names
  * @param blacklist : The list of properties we don't want to show
  * @param show : Whether or not to show the information associated with
  * this pre-processor in the processed workspace's name
  */
  DataPreprocessorAlgorithm(const std::string &name,
                            const std::vector<std::string> &prefix,
                            const std::set<std::string> &blacklist,
                            bool show = true)
      : DataProcessorAlgorithm(name, prefix, blacklist), m_show(show) {

    // A pre-processing algorithm must have two input workspaces
    if (m_inputProperties.size() != 2)
      throw std::invalid_argument("A pre-processing algorithm must have two "
                                  "input workspace properties");
    // And one output workspace
    if (m_outputProperties.size() != 1)
      throw std::invalid_argument(
          "A pre-processing algorithm must have one output workspace property");
  };
  /** Default constructor: use 'Plus' as the default pre-processor algorithm
  */
  DataPreprocessorAlgorithm()
      : DataPreprocessorAlgorithm("Plus", std::vector<std::string>{"TOF_"},
                                  std::set<std::string>{"LHSWorkspace",
                                                        "RHSWorkspace",
                                                        "OutputWorkspace"}){};
  // Destructor
  virtual ~DataPreprocessorAlgorithm(){};
  // The name of the lhs input property
  std::string firstInputProperty() const { return m_inputProperties[0]; };
  // The name of the rhs input property
  std::string secondInputProperty() const { return m_inputProperties[1]; };
  // The name of the output property
  std::string outputProperty() const { return m_outputProperties[0]; };
  // The prefix of the output property
  std::string prefix() const { return m_prefix[0]; };
  // If we want to show the info associated with this pre-processor
  bool show() const { return m_show; };

private:
  // Indicates wheter or not the information will appear in the output ws name
  bool m_show;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H*/
