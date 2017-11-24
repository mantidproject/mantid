#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPreprocessMap.h"

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
*/
DataProcessorPreprocessMap::DataProcessorPreprocessMap() : m_map() {}

// Destructor
DataProcessorPreprocessMap::~DataProcessorPreprocessMap() {}

/** Add a column that needs pre-processing
* @param column :: the name of the column that needs pre-processing
* @param algorithm :: the name of the pre-processing algorithm that will be
* applied to that column
* @param prefix :: a list with the prefix(es) to be added to the output
* workspace(s), as a string
* @param blacklist :: the list of algorithm properties to black list, as a
* string
*/
void DataProcessorPreprocessMap::addElement(const QString &column,
                                            const QString &algorithm,
                                            const QString &prefix,
                                            const QString &blacklist) {

  m_map[column] =
      DataProcessorPreprocessingAlgorithm(algorithm, prefix, blacklist);
}

/** Return a map where keys are columns and values pre-processing algorithms
* @return :: Pre-processing instructions as a map
*/
std::map<QString, DataProcessorPreprocessingAlgorithm>
DataProcessorPreprocessMap::asMap() const {
  return m_map;
}
}
}
