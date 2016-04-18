#ifndef MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHM_H

#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataPostprocessorAlgorithm

DataPostprocessorAlgorithm is an class which defines a post-processor algorithm.
TODO: Description

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
class DataPostprocessorAlgorithm : public DataProcessorAlgorithm {
public:
  /** Constructor
  * @param name : The name of the post-processing algorithm
	* @param prefix : The list of prefixes that will be used for the output
	* workspaces' names
	* @param blacklist : The list of properties we don't want to show
  */
  DataPostprocessorAlgorithm()
      : DataPostprocessorAlgorithm(
            "Stitch1DMany", std::vector<std::string>{"IvsQ"},
            std::set<std::string>{"InputWorkspaces", "OutputWorkspace"}){

        };
  DataPostprocessorAlgorithm(const std::string &name,
                             const std::vector<std::string> &prefix,
                             const std::set<std::string> &blacklist)
      : DataProcessorAlgorithm(name, prefix, blacklist) {

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(m_name);

    auto properties = alg->getProperties();
    for (auto &prop : properties) {

      if (prop->direction() == 0 && prop->type() == "str list") {
        m_inputProperties.push_back(prop->name());
      }
    }

    // A post-processing algorithm must have two input workspaces
    // And one output workspace
    if (m_inputProperties.size() != 1)
      throw std::invalid_argument(
          "A post-processing algorithm must have one input workspace property");
    if (m_outputProperties.size() != 1)
      throw std::invalid_argument("A post-processing algorithm must have one "
                                  "output workspace property");
  };
  // Destructor
  virtual ~DataPostprocessorAlgorithm(){};
  // The name of the input property
  std::string inputProperty() const { return m_inputProperties[0]; };
  // The name of the output property
  std::string outputProperty() const { return m_outputProperties[0]; };
  // The prefix of the output property
  std::string prefix() const { return m_prefix[0]; };
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHM_H*/