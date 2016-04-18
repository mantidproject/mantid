#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHM_H

#include "MantidAPI/AlgorithmManager.h"

#include <set>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataProcessorAlgorithm

DataProcessorAlgorithm is an base class which defines a processing algorithm. 
TODO: description

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
class DataProcessorAlgorithm {
public:
  DataProcessorAlgorithm() = delete;
  /** Constructor
  * @param name : The name of this algorithm
  * @param prefix : The list of prefixes that will be used for the output
  * workspaces' names
  * @param blacklist : The list of properties we do not want to show
  */
  DataProcessorAlgorithm(const std::string &name,
                         const std::vector<std::string> &prefix,
                         const std::set<std::string> &blacklist)
      : m_name(name), m_prefix(prefix), m_blacklist(blacklist) {

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(m_name);

    auto properties = alg->getProperties();
    for (auto &prop : properties) {

      if (prop->direction() == 0 &&
          (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
        // For now, we restrict the input workspaces to either MatrixWorkspace
        // or Workspace
        // This condition can be relaxed if necessary
        m_inputProperties.push_back(prop->name());
      }
      if (prop->direction() == 1 &&
          (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
        // The same for the output workspaces
        m_outputProperties.push_back(prop->name());
      }
    }

    // The number of prefixes given should match the number of output workspaces
    if (m_outputProperties.size() != m_prefix.size()) {
      throw std::invalid_argument("Invalid DataProcessorAlgorithm");
    }
  };
  virtual ~DataProcessorAlgorithm(){};

  // The name of this algorithm
  virtual std::string name() const { return m_name; };
  // The number of output properties
  virtual size_t outputProperties() const { return m_outputProperties.size(); };
  // The prefix for this output property
  virtual std::string prefix(size_t index) const { return m_prefix[index]; };
  // The name of this input property
  virtual std::string inputPropertyName(size_t index) const {
    return m_inputProperties[index];
  };
  // The name of this output property
  virtual std::string outputPropertyName(size_t index) const {
    return m_outputProperties[index];
  };
  // The blacklist
  virtual std::set<std::string> blacklist() const { return m_blacklist; };

protected:
  // The name of this algorithm
  std::string m_name;
  // The prefix of the output workspace(s)
  std::vector<std::string> m_prefix;
  // The names of the input properties
  std::vector<std::string> m_inputProperties;
  // The names of the output properties
  std::vector<std::string> m_outputProperties;
  // The blacklist
  std::set<std::string> m_blacklist;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H*/