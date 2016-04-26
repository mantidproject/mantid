#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORWHITELIST_H
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORWHITELIST_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class DataProcessorWhiteList

DataProcessorWhiteList is an class defining a whitelist

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DataProcessorWhiteList final {
public:
  DataProcessorWhiteList() : m_lastIndex(0){};
  virtual ~DataProcessorWhiteList(){};

  /** Adds an element to the whitelist
  @param colName : the name of the column to be added
  @param algProperty : the name of the property linked to this column
  @param description : a description of this column
  */
  void addElement(const std::string &colName, const std::string &algProperty,
                  const std::string &description = "") {
    m_colIndexToColName[m_lastIndex] = colName;
    m_colIndexToAlgProp[m_lastIndex] = algProperty;
    m_description[m_lastIndex] = description;
    m_colNameToColIndex[colName] = m_lastIndex++;
  };

  int colIndexFromColName(const std::string &colName) const {
    return m_colNameToColIndex.at(colName);
  }
  std::string colNameFromColIndex(int index) const {
    return m_colIndexToColName.at(index);
  }
  std::string algPropFromColIndex(int index) const {
    return m_colIndexToAlgProp.at(index);
  }
  std::string description(int index) const { return m_description.at(index); }

  size_t size() const { return m_colNameToColIndex.size(); }

private:
  int m_lastIndex;
  std::map<std::string, int> m_colNameToColIndex;
  std::map<int, std::string> m_colIndexToColName;
  std::map<int, std::string> m_colIndexToAlgProp;
  std::map<int, std::string> m_description;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_DATAPROCESSORWHITELIST_H*/
