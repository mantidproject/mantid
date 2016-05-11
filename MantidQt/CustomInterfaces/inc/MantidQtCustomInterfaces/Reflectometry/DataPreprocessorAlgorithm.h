#ifndef MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H
#define MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H

#include "MantidAPI/AlgorithmManager.h"

#include <set>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataPreprocessorAlgorithm

DataPreprocessorAlgorithm defines a pre-processor algorithm that will be
responsible for pre-processsing a specific column in a Data Processor UI.

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
class DataPreprocessorAlgorithm {
public:
  /** Constructor
  * @param name : The name of the pre-processing algorithm
  * @param prefix : The prefix that will added to the output workspace name
  * @param blacklist : The list of properties we don't want to show
  * @param show : Whether or not to show the information associated with
  * this pre-processor in the processed workspace's name
  */
  DataPreprocessorAlgorithm(
      const std::string &name, const std::string &prefix = "",
      const std::set<std::string> &blacklist = std::set<std::string>(),
      bool show = true)
      : m_name(name), m_prefix(prefix), m_blacklist(blacklist), m_show(show) {

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(m_name);

    int countInputWS = 0;
    int countOutputWS = 0;

    auto properties = alg->getProperties();
    for (auto &prop : properties) {

      if (prop->direction() == Mantid::Kernel::Direction::Input &&
          (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
        // For now, we restrict the workspaces to either MatrixWorkspace
        // or Workspace, this condition can be relaxed if necessary

        if (countInputWS == 0) {
          m_lhs = prop->name();
        } else {
          m_rhs = prop->name();
        }
        countInputWS++;
      }

      if (prop->direction() == Mantid::Kernel::Direction::Output &&
          (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {

        m_outProperty = prop->name();
				countOutputWS++;
      }
    }

    if (countInputWS != 2) {
      throw std::invalid_argument("Invalid Pre-processing algorithm. A "
                                  "valid algorithm must have two input "
                                  "workspace properties.");
    }
    if (countOutputWS != 1) {
      throw std::invalid_argument("Invalid Pre-processing algorithm. A "
                                  "valid algorithm must have one "
                                  "output workspace property.");
    }
  };
  /** Default constructor: use 'Plus' as the default pre-processor algorithm
  */
  DataPreprocessorAlgorithm()
      : DataPreprocessorAlgorithm("Plus", "TOF_",
                                  std::set<std::string>{"LHSWorkspace",
                                                        "RHSWorkspace",
                                                        "OutputWorkspace"}){};
  // Destructor
  virtual ~DataPreprocessorAlgorithm(){};
  // The name of this algorithm
  std::string name() const { return m_name; };
  // The name of the lhs input property
  std::string lhsProperty() const { return m_lhs; };
  // The name of the rhs input property
  std::string rhsProperty() const { return m_rhs; };
  // The name of the output property
  std::string outputProperty() const { return m_outProperty; };
  // The prefix to add to the output property
  std::string prefix() const { return m_prefix; };
  // If we want to show the info associated with this pre-processor
  bool show() const { return m_show; };
  // The blacklist
  std::set<std::string> blacklist() const { return m_blacklist; };

private:
  // The name of this algorithm
  std::string m_name;
  // The prefix of the output workspace
  std::string m_prefix;
  // The name of the LHS input property
  std::string m_lhs;
  // The name of the RHS input property
  std::string m_rhs;
  // The name of the output proerty
  std::string m_outProperty;
  // The blacklist
  std::set<std::string> m_blacklist;
  // Indicates wheter or not the information will appear in the output ws name
  bool m_show;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHM_H*/
