#include "MantidQtCustomInterfaces/Reflectometry/GenericDataProcessorPresenter.h"

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
}

/** Default constructor: use 'Plus' as the default pre-processor algorithm
*/
DataPreprocessorAlgorithm::DataPreprocessorAlgorithm()
    : DataPreprocessorAlgorithm(
          "Plus", "TOF_", std::set<std::string>{"LHSWorkspace", "RHSWorkspace",
                                                "OutputWorkspace"}) {}
// Destructor
DataPreprocessorAlgorithm::~DataPreprocessorAlgorithm() {}
// Returns the name of this pre-processing algorithm
std::string DataPreprocessorAlgorithm::name() const { return m_name; }
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
// Returns the blacklist
std::set<std::string> DataPreprocessorAlgorithm::blacklist() const {
  return m_blacklist;
}
}
}
