#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Constructor
 * @param name : The name of the pre-processing algorithm
 * @param prefix : A prefix that will added to the output workspace name
 * @param separator : A separator that will added between values in the output
 * workspace name
 * @param blacklist : The list of properties we don't want to show
 * algorithm in the processed workspace's name
 */
PreprocessingAlgorithm::PreprocessingAlgorithm(QString name, QString prefix,
                                               QString separator,
                                               std::set<QString> blacklist)
    : ProcessingAlgorithmBase(std::move(name), std::move(blacklist)),
      m_prefix(std::move(prefix)), m_separator(std::move(separator)) {

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

/** Delegating constructor
 *
 * @param name : The name of the pre-processing algorithm
 * @param prefix : A prefix that will added to the output workspace name
 * @param separator : A separator that will used between values in the output
 * workspace name
 * @param blacklist : The list of properties we don't want to show, as a string
 * algorithm in the processed workspace's name
 */
PreprocessingAlgorithm::PreprocessingAlgorithm(QString name, QString prefix,
                                               QString separator,
                                               const QString &blacklist)
    : PreprocessingAlgorithm(std::move(name), std::move(prefix),
                             std::move(separator),
                             convertStringToSet(blacklist)) {}

/** Default constructor: do nothing
 */
PreprocessingAlgorithm::PreprocessingAlgorithm()
    : m_prefix(), m_separator(), m_lhs(), m_rhs(), m_outProperty() {}

// Destructor
PreprocessingAlgorithm::~PreprocessingAlgorithm() {}

// Returns the name of the lhs input property
QString PreprocessingAlgorithm::lhsProperty() const { return m_lhs; }

// Returns the name of the rhs input property
QString PreprocessingAlgorithm::rhsProperty() const { return m_rhs; }

// Returns the name of the output property
QString PreprocessingAlgorithm::outputProperty() const { return m_outProperty; }

// Returns the prefix to add to the output property
QString PreprocessingAlgorithm::prefix() const { return m_prefix; }

// Returns the separator to separate multiple values in the output property
QString PreprocessingAlgorithm::separator() const { return m_separator; }
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
