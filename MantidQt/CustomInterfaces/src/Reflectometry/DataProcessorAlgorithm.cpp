#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param name : The name of this algorithm
* @param prefix : The list of prefixes that will be used for the output
* workspaces' names
* @param blacklist : The list of properties we do not want to show
*/
DataProcessorAlgorithm::DataProcessorAlgorithm(
    const std::string &name, const std::vector<std::string> &prefix,
    const std::set<std::string> &blacklist)
    : m_name(name), m_prefix(prefix), m_blacklist(blacklist) {

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create(m_name);

  auto properties = alg->getProperties();
  for (auto &prop : properties) {

    if (prop->direction() == Mantid::Kernel::Direction::Input &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
      // For now, we restrict the input workspaces to either MatrixWorkspace
      // or Workspace, this condition can be relaxed if necessary
      m_inputProperties.push_back(prop->name());
    }
    if (prop->direction() == Mantid::Kernel::Direction::Output &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
      // The same for the output workspaces
      m_outputProperties.push_back(prop->name());
    }
  }

  if (!m_inputProperties.size())
    throw std::invalid_argument("Invalid Processing algorithm. A valid "
                                "algorithm must have at least one input "
                                "workpsace property");
  if (!m_outputProperties.size())
    throw std::invalid_argument("Invalid processing algorithm. A valid "
                                "algorithm must have at least one output "
                                "workspace property");

  // The number of prefixes given should match the number of output
  // workspaces
  if (m_outputProperties.size() != m_prefix.size()) {
    throw std::invalid_argument("Invalid DataProcessorAlgorithm");
  }
}
// Destructor
DataProcessorAlgorithm::~DataProcessorAlgorithm() {}
// Returns the name of this algorithm
std::string DataProcessorAlgorithm::name() const { return m_name; }
// Returns the number of output properties
size_t DataProcessorAlgorithm::numberOfOutputProperties() const {
  return m_outputProperties.size();
}
/** Returns the prefix that will be added to the name of this output ws property
 *@param index : The property index
 */
std::string DataProcessorAlgorithm::prefix(size_t index) const {
  return m_prefix[index];
}
/** Returns the name of an input property specified by its index
 *@param index : The property index
 */
std::string DataProcessorAlgorithm::inputPropertyName(size_t index) const {
  return m_inputProperties[index];
}
/** Returns the name of an output ws property specified by its index
 *@param index : The property index
 */
std::string DataProcessorAlgorithm::outputPropertyName(size_t index) const {
  return m_outputProperties[index];
}
// Returns the blacklist
std::set<std::string> DataProcessorAlgorithm::blacklist() const {
  return m_blacklist;
}
}
}
