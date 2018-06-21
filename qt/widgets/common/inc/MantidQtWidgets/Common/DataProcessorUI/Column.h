#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLUMN_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLUMN_H
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class Column

A column represents a whitelist element providing easy access to it's name,
algorithm, visibility status, prefix and description.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class EXPORT_OPT_MANTIDQT_COMMON Column {
public:
  Column(QString const &name, QString const &algorithmProperty, bool isShown,
         QString const &prefix, QString const &description, bool isKey);
  QString const &name() const;
  QString const &algorithmProperty() const;
  bool isShown() const;
  bool isKey() const;
  QString const &prefix() const;
  QString const &description() const;

private:
  QString const &m_name;
  QString const &m_algorithmProperty;
  bool m_isShown;
  QString const &m_prefix;
  QString const &m_description;
  bool m_isKey;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLUMN_H
