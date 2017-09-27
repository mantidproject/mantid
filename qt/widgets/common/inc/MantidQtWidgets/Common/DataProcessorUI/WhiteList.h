#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORWHITELIST_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORWHITELIST_H

#include "MantidQtWidgets/Common/DataProcessorUI/Column.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ConstColumnIterator.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <map>
#include <vector>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class WhiteList

WhiteList is an class defining a whitelist

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

class EXPORT_OPT_MANTIDQT_COMMON WhiteList {
public:
  using const_iterator = ConstColumnIterator;
  virtual ~WhiteList(){};

  void addElement(const QString &colName, const QString &algProperty,
                  const QString &description, bool showValue = false,
                  const QString &prefix = "");
  int indexFromName(const QString &colName) const;
  QString name(int index) const;
  QString algorithmProperty(int index) const;
  QString description(int index) const;
  QString prefix(int index) const;
  bool isShown(int index) const;
  std::size_t size() const;
  const_iterator cbegin() const;
  const_iterator begin() const;
  const_iterator cend() const;
  const_iterator end() const;
  std::vector<QString> const &names() const;

private:
  std::vector<QString> m_names;
  std::vector<QString> m_algorithmProperties;
  std::vector<bool> m_isShown;
  std::vector<QString> m_prefixes;
  std::vector<QString> m_descriptions;
};
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORWHITELIST_H*/
