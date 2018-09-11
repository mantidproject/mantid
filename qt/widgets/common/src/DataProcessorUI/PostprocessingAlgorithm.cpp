#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Constructor
 * @param name : The name of the post-processing algorithm
 * @param prefix : The prefix that will be added to the output workspace name
 * @param blacklist : The list of properties we don't want to show
 */
PostprocessingAlgorithm::PostprocessingAlgorithm(
    const QString &name, const QString &prefix,
    const std::set<QString> &blacklist)
    : ProcessingAlgorithmBase(name, blacklist), m_prefix(prefix) {

  auto inputStrListProperties = getInputStrListProperties();
  if (inputStrListProperties.size() != 1)
    throw std::invalid_argument("Invalid post-processing algorithm. A "
                                "valid algorithm must have one input "
                                "'str list' property");

  m_inputProp = inputStrListProperties.at(0);

  auto outputWsProperties = getOutputWsProperties();
  if (outputWsProperties.size() != 1)
    throw std::invalid_argument("Invalid post-processing algorithm. A "
                                "valid algorithm must have one output "
                                "workspace property");

  m_outputProp = outputWsProperties.at(0);
}

/** Delegating constructor
 * @param name : The name of the post-processing algorithm
 * @param prefix : The prefix that will be added to the output workspace name
 * @param blacklist : The list of properties we don't want to show, as a string
 */
PostprocessingAlgorithm::PostprocessingAlgorithm(const QString &name,
                                                 const QString &prefix,
                                                 const QString &blacklist)
    : PostprocessingAlgorithm(name, prefix, convertStringToSet(blacklist)) {}

/** Default constructor: no algorithm defined */
PostprocessingAlgorithm::PostprocessingAlgorithm()
    : m_prefix(), m_inputProp(), m_outputProp() {}

// Destructor
PostprocessingAlgorithm::~PostprocessingAlgorithm() {}

// Returns the name of the input workspace property
QString PostprocessingAlgorithm::inputProperty() const { return m_inputProp; }

// Returns the name of the output workspace property
QString PostprocessingAlgorithm::outputProperty() const { return m_outputProp; }

// Returns the number of output workspace properties (currently only 1)
size_t PostprocessingAlgorithm::numberOfOutputProperties() const { return 1; }

// Returns the prefix that will be added to the output ws
QString PostprocessingAlgorithm::prefix() const { return m_prefix; }
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
