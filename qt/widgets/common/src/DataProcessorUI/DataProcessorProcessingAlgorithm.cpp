#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorProcessingAlgorithm.h"

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
* @param name : The name of this algorithm
* @param prefix : The list of prefixes that will be used for the output
* workspaces' names
* @param blacklist : The list of properties we do not want to show
*/
DataProcessorProcessingAlgorithm::DataProcessorProcessingAlgorithm(
    const QString &name, const std::vector<QString> &prefix,
    const std::set<QString> &blacklist)
    : DataProcessorProcessingAlgorithmBase(name, blacklist), m_prefix(prefix) {

  m_inputProperties = getInputWsProperties();
  if (!m_inputProperties.size())
    throw std::invalid_argument("Invalid Processing algorithm. A valid "
                                "algorithm must have at least one input "
                                "workpsace property");

  m_outputProperties = getOutputWsProperties();
  if (!m_outputProperties.size())
    throw std::invalid_argument("Invalid processing algorithm. A valid "
                                "algorithm must have at least one output "
                                "workspace property");

  // The number of prefixes given should match the number of output
  // workspaces
  if (m_outputProperties.size() != m_prefix.size()) {
    throw std::invalid_argument(
        "Invalid DataProcessorProcessingAlgorithm. The number of prefixes "
        "given must "
        "match the number of output ws properties defined for this algorithm");
  }
}

/** Delegating constructor
* @param name : The name of this algorithm
* @param prefix : The list of prefixes that will be used for the output
* workspaces' names, as a string
* @param blacklist : The list of properties we do not want to show, as a string
*/
DataProcessorProcessingAlgorithm::DataProcessorProcessingAlgorithm(
    const QString &name, const QString &prefix, const QString &blacklist)
    : DataProcessorProcessingAlgorithm(name, convertStringToVector(prefix),
                                       convertStringToSet(blacklist)) {}

/**
 * Constructor
*/
DataProcessorProcessingAlgorithm::DataProcessorProcessingAlgorithm()
    : m_prefix(), m_inputProperties(), m_outputProperties() {}

// Destructor
DataProcessorProcessingAlgorithm::~DataProcessorProcessingAlgorithm() {}

// Returns the number of output properties
size_t DataProcessorProcessingAlgorithm::numberOfOutputProperties() const {
  return m_outputProperties.size();
}

/** Returns the prefix that will be added to the name of this output ws property
 *@param index : The property index
 */
QString DataProcessorProcessingAlgorithm::prefix(size_t index) const {
  return m_prefix[index];
}

/** Returns the name of an input property specified by its index
 *@param index : The property index
 */
QString
DataProcessorProcessingAlgorithm::inputPropertyName(size_t index) const {
  return m_inputProperties[index];
}

/** Returns the name of an output ws property specified by its index
 *@param index : The property index
 */
QString
DataProcessorProcessingAlgorithm::outputPropertyName(size_t index) const {
  return m_outputProperties[index];
}
}
}
