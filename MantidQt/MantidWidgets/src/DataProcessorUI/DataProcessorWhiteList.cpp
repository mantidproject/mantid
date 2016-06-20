#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"

namespace MantidQt {
namespace MantidWidgets {

/** Adds an element to the whitelist
    @param colName : the name of the column to be added
    @param algProperty : the name of the property linked to this column
    @param description : a description of this column
*/
void DataProcessorWhiteList::addElement(const std::string &colName,
                                        const std::string &algProperty,
                                        const std::string &description) {
  m_colIndexToColName[m_lastIndex] = colName;
  m_colIndexToAlgProp[m_lastIndex] = algProperty;
  m_description[m_lastIndex] = description;
  m_colNameToColIndex[colName] = m_lastIndex++;
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
}
}
