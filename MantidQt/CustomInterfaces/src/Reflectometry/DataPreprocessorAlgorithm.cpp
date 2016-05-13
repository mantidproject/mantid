#include "MantidQtCustomInterfaces/Reflectometry/DataPreprocessorAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param name : The name of the pre-processing algorithm
 * @param prefix : The prefix that will added to the output workspace name
 * @param blacklist : The list of properties we don't want to show
 * @param show : Whether or not to show the information associated with
 * this pre-processor in the processed workspace's name
 */
DataPreprocessorAlgorithm::DataPreprocessorAlgorithm(
    const std::string &name, const std::string &prefix,
    const std::set<std::string> &blacklist, bool show)
    : DataProcessorAlgorithmBase(name, blacklist), m_prefix(prefix), m_show(show) {

  auto inputWsProperties = getInputWsProperties();

  if (inputWsProperties.size() != 2) {
    throw std::invalid_argument("Invalid Pre-processing algorithm. A "
                                "valid algorithm must have two input "
                                "workspace properties.");
  }
  m_lhs = inputWsProperties.at(0);
  m_rhs = inputWsProperties.at(1);

  auto outputWsProperties = getOutputWsProperties();

  if (outputWsProperties.size() != 1) {
    throw std::invalid_argument("Invalid Pre-processing algorithm. A "
                                "valid algorithm must have one "
                                "output workspace property.");
  }
  m_outProperty = outputWsProperties.at(0);
}

/** Default constructor: use 'Plus' as the default pre-processor algorithm
*/
DataPreprocessorAlgorithm::DataPreprocessorAlgorithm()
    : DataPreprocessorAlgorithm(
          "Plus", "TOF_", std::set<std::string>{"LHSWorkspace", "RHSWorkspace",
                                                "OutputWorkspace"}) {}

// Destructor
DataPreprocessorAlgorithm::~DataPreprocessorAlgorithm() {}

// Returns the name of the lhs input property
std::string DataPreprocessorAlgorithm::lhsProperty() const { return m_lhs; }

// Returns the name of the rhs input property
std::string DataPreprocessorAlgorithm::rhsProperty() const { return m_rhs; }

// Returns the name of the output property
std::string DataPreprocessorAlgorithm::outputProperty() const {
  return m_outProperty;
}

// Returns the prefix to add to the output property
std::string DataPreprocessorAlgorithm::prefix() const { return m_prefix; }

// Returns a boolean indicating whether or not we want to add the prefix
// associated to this pre-processor to the output workspace name
bool DataPreprocessorAlgorithm::show() const { return m_show; }
}
}
