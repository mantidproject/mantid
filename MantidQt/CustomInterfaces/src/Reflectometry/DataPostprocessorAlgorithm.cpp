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
    : m_name(name), m_prefix(prefix), m_blacklist(blacklist) {

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create(m_name);

  int countInputWS = 0;
  int countOutputWS = 0;

  auto properties = alg->getProperties();
  for (auto &prop : properties) {

    if (prop->direction() == Mantid::Kernel::Direction::Input &&
        prop->type() == "str list") {
      // For now, we assume we receive the list of workspace to post-process
      // as
      // a 'str list'
      m_inputProp = prop->name();

      countInputWS++;
    }
    if (prop->direction() == Mantid::Kernel::Direction::Output &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {
      // For now, we restrict the output workspaces to either MatrixWorkspace
      // or Worksace
      m_outputProp = prop->name();

      countOutputWS++;
    }
  }

  if (countInputWS != 1)
    throw std::invalid_argument("Invalid post-processing algorithm. A "
                                "valid algorithm must have one input "
                                "'str list' property");
  if (countOutputWS != 1)
    throw std::invalid_argument("Invalid post-processing algorithm. A "
                                "valid algorithm must have one output "
                                "workspace property");
}
/** Default constructor: use 'Stitch1DMany' as the default post-processor
 * algorithm */
DataPostprocessorAlgorithm::DataPostprocessorAlgorithm()
    : DataPostprocessorAlgorithm(
          "Stitch1DMany", "IvsQ_",
          std::set<std::string>{"InputWorkspaces", "OutputWorkspace"}) {}
// Destructor
DataPostprocessorAlgorithm::~DataPostprocessorAlgorithm() {}
// Returns the name of this algorithm
std::string DataPostprocessorAlgorithm::name() const { return m_name; }
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
// Returns the blacklist
std::set<std::string> DataPostprocessorAlgorithm::blacklist() const {
  return m_blacklist;
}
}
}
