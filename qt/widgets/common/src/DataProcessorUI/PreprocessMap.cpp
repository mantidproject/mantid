#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessMap.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Constructor
 */
PreprocessMap::PreprocessMap() : m_map() {}

// Destructor
PreprocessMap::~PreprocessMap() {}

/** Add a column that needs pre-processing
 * @param column :: the name of the column that needs pre-processing
 * @param algorithm :: the name of the pre-processing algorithm that will be
 * applied to that column
 * @param prefix :: a list with the prefix(es) to be added to the output
 * workspace(s), as a string
 * @param separator :: the separator to use between elements of the output
 * workspace name
 * @param blacklist :: the list of algorithm properties to black list, as a
 * string
 */
void PreprocessMap::addElement(const QString &column, const QString &algorithm,
                               const QString &prefix, const QString &separator,
                               const QString &blacklist) {

  m_map[column] =
      PreprocessingAlgorithm(algorithm, prefix, separator, blacklist);
}

/** Return a map where keys are columns and values pre-processing algorithms
 * @return :: Pre-processing instructions as a map
 */
std::map<QString, PreprocessingAlgorithm> PreprocessMap::asMap() const {
  return m_map;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
