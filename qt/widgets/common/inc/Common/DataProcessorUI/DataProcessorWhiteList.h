#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORWHITELIST_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORWHITELIST_H

#include "MantidQtMantidWidgets/WidgetDllOption.h"

#include <map>
#include <vector>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorWhiteList {
public:
  DataProcessorWhiteList() : m_lastIndex(0){};
  virtual ~DataProcessorWhiteList(){};

  void addElement(const QString &colName, const QString &algProperty,
                  const QString &description, bool showValue = false,
                  const QString &prefix = "");
  int colIndexFromColName(const std::string &colName) const;
  std::string colNameFromColIndex(int index) const;
  std::string algPropFromColIndex(int index) const;
  std::string description(int index) const;
  std::string prefix(int index) const;
  bool showValue(int index) const;
  size_t size() const;

private:
  int m_lastIndex;
  std::map<std::string, int> m_colNameToColIndex;
  std::vector<std::string> m_colIndexToColName;
  std::vector<std::string> m_colIndexToAlgProp;
  std::vector<bool> m_showValue;
  std::vector<std::string> m_prefix;
  std::vector<std::string> m_description;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORWHITELIST_H*/
