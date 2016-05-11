#ifndef MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHM_H

#include "MantidAPI/AlgorithmManager.h"

#include <set>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataPostprocessorAlgorithm

DataPostprocessorAlgorithm defines a post-processor algorithm responsible for
post-processing rows belonging to the same group in a Data Processor UI.

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
class DataPostprocessorAlgorithm {
public:
  /** Constructor
  * @param name : The name of the post-processing algorithm
  * @param prefix : The prefix that will be added to the output workspace name
  * @param blacklist : The list of properties we don't want to show
  */
  DataPostprocessorAlgorithm(
      const std::string &name, const std::string &prefix = "",
      const std::set<std::string> &blacklist = std::set<std::string>())
      : m_name(name), m_prefix(prefix), m_blacklist(blacklist) {

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(m_name);

    int countInputWS = 0;
    int countOutputWS = 0;

    auto properties = alg->getProperties();
    for (auto &prop : properties) {

      if (prop->direction() == Mantid::Kernel::Direction::Input &&
          prop->type() == "str list") {
        // For now, we assume we receive the list of workspace to post-process
        // as
        // a 'str list'
        m_inputProp = prop->name();

        countInputWS++;
      }
      if (prop->direction() == Mantid::Kernel::Direction::Output &&
          (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
        // For now, we restrict the output workspaces to either MatrixWorkspace
        // or Worksace
        m_outputProp = prop->name();

        countOutputWS++;
      }
    }

    if (countInputWS != 1)
      throw std::invalid_argument("Invalid post-processing algorithm. A "
                                  "valid algorithm must have one input "
                                  "'str list' property");
    if (countOutputWS != 1)
      throw std::invalid_argument("Invalid post-processing algorithm. A "
                                  "valid algorithm must have one output "
                                  "workspace property");
  };
  /** Default constructor: use 'Stitch1DMany' as the default post-processor
   * algorithm */
  DataPostprocessorAlgorithm()
      : DataPostprocessorAlgorithm(
            "Stitch1DMany", "IvsQ_",
            std::set<std::string>{"InputWorkspaces", "OutputWorkspace"}){};
  // Destructor
  virtual ~DataPostprocessorAlgorithm(){};
  // The name of this algorithm
  std::string name() const { return m_name; };
  // The name of the input workspace property
  std::string inputProperty() const { return m_inputProp; };
  // The name of the output workspace property
  std::string outputProperty() const { return m_outputProp; };
  // The number of output workspace properties (currently only 1)
  size_t numberOfOutputProperties() const { return 1; };
  // The prefix of the output property
  std::string prefix() const { return m_prefix; };
  // The blacklist
  std::set<std::string> blacklist() const { return m_blacklist; };

private:
  // The name of this algorithm
  std::string m_name;
  // The prefix of the output workspace
  std::string m_prefix;
  // The name of the input property
  std::string m_inputProp;
  // The name of the output property
  std::string m_outputProp;
  // The blacklist
  std::set<std::string> m_blacklist;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHM_H*/