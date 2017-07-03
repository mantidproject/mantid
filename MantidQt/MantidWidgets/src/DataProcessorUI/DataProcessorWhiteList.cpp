#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

/** Adds an element to the whitelist
* @param colName : the name of the column to be added
* @param algProperty : the name of the property linked to this column
* @param showValue : true if we want to use what's in this column to generate
* the output ws name
* @param prefix : the prefix to be added to the value of this column
* @param description : a description of this column
*/
void DataProcessorWhiteList::addElement(const QString &colName,
                                        const QString &algProperty,
                                        const QString &description,
                                        bool showValue, const QString &prefix) {

  m_colIndexToColName.push_back(colName.toStdString());
  m_colIndexToAlgProp.push_back(algProperty.toStdString());
  m_showValue.push_back(showValue);
  m_prefix.push_back(prefix.toStdString());
  m_description.push_back(description.toStdString());
  m_colNameToColIndex[colName.toStdString()] = m_lastIndex++;
}

/** Returns the column index for a column specified via its name
    @param colName : The column name
*/
int DataProcessorWhiteList::colIndexFromColName(
    const std::string &colName) const {
  return m_colNameToColIndex.at(colName);
}

/** Returns the column name for a column specified via its index
    @param index : The column index
*/
std::string DataProcessorWhiteList::colNameFromColIndex(int index) const {
  return m_colIndexToColName.at(index);
}

/** Returns the algorithm property linked to a column specified via its index
    @param index : The column index
*/
std::string DataProcessorWhiteList::algPropFromColIndex(int index) const {
  return m_colIndexToAlgProp.at(index);
}

/** Returns the column description for a column specified via its index
    @param index : The column index
*/
std::string DataProcessorWhiteList::description(int index) const {
  return m_description.at(index);
}

/** Returns the size of this whitelist, i.e. the number of columns
*/
size_t DataProcessorWhiteList::size() const {
  return m_colNameToColIndex.size();
}

/** Returns true if the contents of this column should be used to generate the
 * name of the output ws
 * @param index : The column index
*/
bool DataProcessorWhiteList::showValue(int index) const {
  return m_showValue.at(index);
}

/** Returns the column prefix used to generate the name of the output ws (will
* only be used if showValue is true for this column
* @param index : The column index
*/
std::string DataProcessorWhiteList::prefix(int index) const {
  return m_prefix.at(index);
}
}
}
