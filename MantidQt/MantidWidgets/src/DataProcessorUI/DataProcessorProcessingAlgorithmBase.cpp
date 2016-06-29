#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h"

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

} // namespace MantidWidgets
} // namespace Mantid
