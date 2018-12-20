#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithmBase.h"
#include <QStringList>
#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Constructor */
ProcessingAlgorithmBase::ProcessingAlgorithmBase(
    const QString &name, const std::set<QString> &blacklist)
    : m_algName(name), m_blacklist(blacklist), m_inputWsProperties(),
      m_inputStrListProperties(), m_OutputWsProperties() {

  countWsProperties();
}

/** Default constructor (nothing to do) */
ProcessingAlgorithmBase::ProcessingAlgorithmBase()
    : m_algName(), m_blacklist(), m_inputWsProperties(),
      m_inputStrListProperties(), m_OutputWsProperties() {}

/** Destructor */
ProcessingAlgorithmBase::~ProcessingAlgorithmBase() {}

/** Counts the number of input/output workspace properties */
void ProcessingAlgorithmBase::countWsProperties() {

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create(m_algName.toStdString());

  auto properties = alg->getProperties();
  for (auto &prop : properties) {

    if (prop->direction() == Mantid::Kernel::Direction::Input &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace" ||
         prop->type() == "Workspace2D")) {

      m_inputWsProperties.push_back(QString::fromStdString(prop->name()));
    }
    if (prop->direction() == Mantid::Kernel::Direction::Input &&
        prop->type() == "str list") {

      m_inputStrListProperties.push_back(QString::fromStdString(prop->name()));
    }
    if (prop->direction() == Mantid::Kernel::Direction::Output &&
        (prop->type() == "MatrixWorkspace" || prop->type() == "Workspace")) {

      m_OutputWsProperties.push_back(QString::fromStdString(prop->name()));
    }
  }
}

// Returns the input workspaces properties defined for this algorithm
std::vector<QString> ProcessingAlgorithmBase::getInputWsProperties() {
  return m_inputWsProperties;
}
// Returns the input str list properties defined for this algorithm
std::vector<QString> ProcessingAlgorithmBase::getInputStrListProperties() {
  return m_inputStrListProperties;
}
// Returns the output workspaces properties defined for this algorithm
std::vector<QString> ProcessingAlgorithmBase::getOutputWsProperties() {
  return m_OutputWsProperties;
}

/** Converts a string to a vector of strings. Input string will be split by
 * commas.
 * @param text :: the input string to convert
 * @return :: the string as a vector
 */
std::vector<QString>
ProcessingAlgorithmBase::convertStringToVector(const QString &text) {

  if (text.isEmpty())
    return std::vector<QString>();

  auto items = text.split(QChar(','), QString::SkipEmptyParts);
  return std::vector<QString>(items.begin(), items.end());
}

/** Converts a string to a set of strings. Input string will be split by commas.
 * @param text :: the input string to convert
 * @return :: the string as a set
 */
std::set<QString>
ProcessingAlgorithmBase::convertStringToSet(const QString &text) {

  if (text.isEmpty())
    return std::set<QString>();

  auto items = text.split(QChar(','), QString::SkipEmptyParts);
  std::set<QString> out(items.begin(), items.end());
  return out;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
