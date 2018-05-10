#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Convert an options map from a QMap to a std::map
 * @param src [in] : the options as a map of the property name
 * to the property value
 * @return : the options as a std::map of string key to string value
 */
OptionsMap convertOptionsFromQMap(const OptionsQMap &src) {
  OptionsMap dest;
  // Loop through all values in the options QMap
  for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
    const auto propertyName = iter.key();
    const auto valueVariant = iter.value();
    const auto valueStr = valueVariant.value<QString>();
    dest[propertyName] = valueStr;
  }
  return dest;
}

/** Convert a column options map from a QMap to a std::map
 * @param src [in] : the per-column options as a map of the column name
 * to a map of the options applicable to that column
 * @return : the column options as a std::map
 */
ColumnOptionsMap convertColumnOptionsFromQMap(const ColumnOptionsQMap &src) {
  ColumnOptionsMap dest;
  // Loop through all columns in the QMap
  for (auto columnIter = src.constBegin(); columnIter != src.constEnd();
       ++columnIter) {
    // Convert the options QMap into a std::map and set it as the value
    // for this column
    const auto columnName = columnIter.key();
    const auto optionsVariant = columnIter.value();
    const auto optionsQMap = optionsVariant.value<OptionsQMap>();
    dest[columnName] = convertOptionsFromQMap(optionsQMap);
  }
  return dest;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
