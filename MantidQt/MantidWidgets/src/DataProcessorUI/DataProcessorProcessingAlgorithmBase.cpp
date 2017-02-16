#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h"
#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace MantidWidgets {

/** Constructor */
DataProcessorProcessingAlgorithmBase::DataProcessorProcessingAlgorithmBase(
    const std::string &name, const std::set<std::string> &blacklist)
    : m_algName(name), m_blacklist(blacklist), m_inputWsProperties(),
      m_inputStrListProperties(), m_OutputWsProperties() {

  countWsProperties();
}

/** Default constructor (nothing to do) */
DataProcessorProcessingAlgorithmBase::DataProcessorProcessingAlgorithmBase()
    : m_algName(), m_blacklist(), m_inputWsProperties(),
      m_inputStrListProperties(), m_OutputWsProperties() {}

/** Destructor */
DataProcessorProcessingAlgorithmBase::~DataProcessorProcessingAlgorithmBase() {}

/** Counts the number of input/output workspace properties */
void DataProcessorProcessingAlgorithmBase::countWsProperties() {

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create(m_algName);

  auto properties = alg->getProperties();
  for (auto &prop : properties) {

    if (prop->direction() == Mantid::Kernel::Direction::Input &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace" ||
         prop->type() == "Workspace2D")) {

      m_inputWsProperties.push_back(prop->name());
    }
    if (prop->direction() == Mantid::Kernel::Direction::Input &&
        prop->type() == "str list") {

      m_inputStrListProperties.push_back(prop->name());
    }
    if (prop->direction() == Mantid::Kernel::Direction::Output &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {

      m_OutputWsProperties.push_back(prop->name());
    }
  }
}

// Returns the input workspaces properties defined for this algorithm
std::vector<std::string>
DataProcessorProcessingAlgorithmBase::getInputWsProperties() {
  return m_inputWsProperties;
}
// Returns the input str list properties defined for this algorithm
std::vector<std::string>
DataProcessorProcessingAlgorithmBase::getInputStrListProperties() {
  return m_inputStrListProperties;
}
// Returns the output workspaces properties defined for this algorithm
std::vector<std::string>
DataProcessorProcessingAlgorithmBase::getOutputWsProperties() {
  return m_OutputWsProperties;
}

/** Converts a string to a vector of strings. Input string will be split by
* commas.
* @param text :: the input string to convert
* @return :: the string as a vector
*/
std::vector<std::string>
DataProcessorProcessingAlgorithmBase::convertStringToVector(
    const std::string &text) {

  if (text.empty())
    return std::vector<std::string>();

  std::vector<std::string> vec;
  boost::split(vec, text, boost::is_any_of(","));
  return vec;
}

/** Converts a string to a set of strings. Input string will be split by commas.
* @param text :: the input string to convert
* @return :: the string as a set
*/
std::set<std::string> DataProcessorProcessingAlgorithmBase::convertStringToSet(
    const std::string &text) {

  if (text.empty())
    return std::set<std::string>();

  std::vector<std::string> vec;
  boost::split(vec, text, boost::is_any_of(","));

  std::set<std::string> out(vec.begin(), vec.end());
  return out;
}
} // namespace MantidWidgets
} // namespace Mantid
