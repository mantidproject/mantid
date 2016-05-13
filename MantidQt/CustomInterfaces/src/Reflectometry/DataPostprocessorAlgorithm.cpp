#include "MantidQtCustomInterfaces/Reflectometry/DataPostprocessorAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param name : The name of the post-processing algorithm
* @param prefix : The prefix that will be added to the output workspace name
* @param blacklist : The list of properties we don't want to show
*/
DataPostprocessorAlgorithm::DataPostprocessorAlgorithm(
    const std::string &name, const std::string &prefix,
    const std::set<std::string> &blacklist)
    : DataProcessorAlgorithmBase(name, blacklist), m_prefix(prefix) {

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
/** Default constructor: use 'Stitch1DMany' as the default post-processor
 * algorithm */
DataPostprocessorAlgorithm::DataPostprocessorAlgorithm()
    : DataPostprocessorAlgorithm(
          "Stitch1DMany", "IvsQ_",
          std::set<std::string>{"InputWorkspaces", "OutputWorkspace"}) {}
// Destructor
DataPostprocessorAlgorithm::~DataPostprocessorAlgorithm() {}

// Returns the name of the input workspace property
std::string DataPostprocessorAlgorithm::inputProperty() const {
  return m_inputProp;
}

// Returns the name of the output workspace property
std::string DataPostprocessorAlgorithm::outputProperty() const {
  return m_outputProp;
}

// Returns the number of output workspace properties (currently only 1)
size_t DataPostprocessorAlgorithm::numberOfOutputProperties() const {
  return 1;
}

// Returns the prefix that will be added to the output ws
std::string DataPostprocessorAlgorithm::prefix() const { return m_prefix; }
}
}
