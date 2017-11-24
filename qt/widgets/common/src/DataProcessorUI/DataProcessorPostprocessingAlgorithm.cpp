#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
* @param name : The name of the post-processing algorithm
* @param prefix : The prefix that will be added to the output workspace name
* @param blacklist : The list of properties we don't want to show
*/
DataProcessorPostprocessingAlgorithm::DataProcessorPostprocessingAlgorithm(
    const QString &name, const QString &prefix,
    const std::set<QString> &blacklist)
    : DataProcessorProcessingAlgorithmBase(name, blacklist), m_prefix(prefix) {

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
DataProcessorPostprocessingAlgorithm::DataProcessorPostprocessingAlgorithm(
    const QString &name, const QString &prefix, const QString &blacklist)
    : DataProcessorPostprocessingAlgorithm(name, prefix,
                                           convertStringToSet(blacklist)) {}

/** Default constructor: no algorithm defined */
DataProcessorPostprocessingAlgorithm::DataProcessorPostprocessingAlgorithm()
    : m_prefix(), m_inputProp(), m_outputProp() {}

// Destructor
DataProcessorPostprocessingAlgorithm::~DataProcessorPostprocessingAlgorithm() {}

// Returns the name of the input workspace property
QString DataProcessorPostprocessingAlgorithm::inputProperty() const {
  return m_inputProp;
}

// Returns the name of the output workspace property
QString DataProcessorPostprocessingAlgorithm::outputProperty() const {
  return m_outputProp;
}

// Returns the number of output workspace properties (currently only 1)
size_t DataProcessorPostprocessingAlgorithm::numberOfOutputProperties() const {
  return 1;
}

// Returns the prefix that will be added to the output ws
QString DataProcessorPostprocessingAlgorithm::prefix() const {
  return m_prefix;
}
}
}
