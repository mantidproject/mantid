#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Constructor
* @param name : The name of this algorithm
* @param prefix : The list of prefixes that will be used for the output
* workspaces' names
* @param postprocessedOutputPrefixIndex The zero based index of the prefix for
* the workspace which should be postprocessed
* @param blacklist : The list of properties we do not want to show
*/
ProcessingAlgorithm::ProcessingAlgorithm(
    QString name, std::vector<QString> prefix,
    std::size_t postprocessedOutputPrefixIndex, std::set<QString> blacklist)
    : ProcessingAlgorithmBase(std::move(name), std::move(blacklist)),
      m_postprocessedOutputPrefixIndex(postprocessedOutputPrefixIndex),
      m_prefix(std::move(prefix)) {

  ensureValidPostprocessedOutput();

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
        "Invalid ProcessingAlgorithm. The number of prefixes "
        "given must "
        "match the number of output ws properties defined for this algorithm");
  }
}

/** Delegating constructor
* @param name : The name of this algorithm
* @param prefix : The list of prefixes that will be used for the output
* workspaces' names, as a string
* @param postprocessedOutputPrefixIndex The zero based index of the prefix for
* the workspace which should be postprocessed
* @param blacklist : The list of properties we do not want to show, as a string
*/
ProcessingAlgorithm::ProcessingAlgorithm(
    QString name, QString const &prefix,
    std::size_t postprocessedOutputPrefixIndex, QString const &blacklist)
    : ProcessingAlgorithm(std::move(name), convertStringToVector(prefix),
                          postprocessedOutputPrefixIndex,
                          convertStringToSet(blacklist)) {}

/**
 * Constructor
*/
ProcessingAlgorithm::ProcessingAlgorithm()
    : m_prefix(), m_inputProperties(), m_outputProperties() {}

// Destructor
ProcessingAlgorithm::~ProcessingAlgorithm() {}

// Returns the number of output properties
size_t ProcessingAlgorithm::numberOfOutputProperties() const {
  return m_outputProperties.size();
}

/** Returns the prefix that will be added to the name of this output ws property
 *@param index : The property index
 */
QString ProcessingAlgorithm::prefix(size_t index) const {
  return m_prefix[index];
}

/** Returns the name of an input property specified by its index
 *@param index : The property index
 */
QString ProcessingAlgorithm::inputPropertyName(size_t index) const {
  return m_inputProperties[index];
}

/** Returns the name of an output ws property specified by its index
 *@param index : The property index
 */
QString ProcessingAlgorithm::outputPropertyName(size_t index) const {
  return m_outputProperties[index];
}

/** Returns the prefix that will be added to the default output ws property
 */
QString ProcessingAlgorithm::defaultOutputPrefix() const { return m_prefix[0]; }

/** Returns the default output ws property. This is just the first
 * property declared by the algorithm. Algorithm properties are
 * extracted in order, so this is the first in our list.
 */
QString ProcessingAlgorithm::defaultOutputPropertyName() const {
  return m_outputProperties[0];
}

bool ProcessingAlgorithm::isValidOutputPrefixIndex(
    std::size_t outputPrefixIndex) const {
  return outputPrefixIndex < m_prefix.size();
}

void ProcessingAlgorithm::ensureValidPostprocessedOutput() const {
  if (!isValidOutputPrefixIndex(m_postprocessedOutputPrefixIndex))
    throw std::runtime_error("Postprocessed output index must be a valid index "
                             "into the prefix array.");
}

QString ProcessingAlgorithm::postprocessedOutputPrefix() const {
  return m_prefix[m_postprocessedOutputPrefixIndex];
}

/** Returns the postprocessed output ws property. This is property
 * name specified on construction.
 */
QString ProcessingAlgorithm::postprocessedOutputPropertyName() const {
  return m_outputProperties[m_postprocessedOutputPrefixIndex];
}

/** Returns the default input ws property. This is just the first
 * property declared by the algorithm. Algorithm properties are
 * extracted in order, so this is the first in our list.
 */
QString ProcessingAlgorithm::defaultInputPropertyName() const {
  return m_inputProperties[0];
}

/** Returns the list of input property names
 */
std::vector<QString> ProcessingAlgorithm::inputProperties() const {
  return m_inputProperties;
}

/** Returns the list of output property names
 */
std::vector<QString> ProcessingAlgorithm::outputProperties() const {
  return m_outputProperties;
}

/** Returns the list of prefixes associated with the output properties
 */
std::vector<QString> ProcessingAlgorithm::prefixes() const { return m_prefix; }
}
}
}
