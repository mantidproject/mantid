#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCONSTCOLUMNITERATOR_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCONSTCOLUMNITERATOR_H
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Column.h"
#include <QString>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ConstColumnIterator

The const column iterator is a ForwardIterator for iterating over several
columns
who's attributes may be stored separately.

It is currently used to allow easy iteration over a WhiteList.

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
class EXPORT_OPT_MANTIDQT_COMMON ConstColumnIterator {
  using QStringIterator = std::vector<QString>::const_iterator;
  using BoolIterator = std::vector<bool>::const_iterator;

public:
  using iterator_category = std::forward_iterator_tag;
  using reference = const Column;
  using pointer = const Column *;
  using value_type = const Column;
  using difference_type = typename QStringIterator::difference_type;
  ConstColumnIterator(QStringIterator names, QStringIterator descriptions,
                      QStringIterator algorithmProperties, BoolIterator isShown,
                      QStringIterator prefixes, BoolIterator isKey);

  ConstColumnIterator &operator++();
  ConstColumnIterator operator++(int);
  reference operator*() const;
  bool operator==(const ConstColumnIterator &other) const;
  bool operator!=(const ConstColumnIterator &other) const;
  ConstColumnIterator &operator+=(difference_type n);
  ConstColumnIterator &operator-=(difference_type n);

private:
  QStringIterator m_names;
  QStringIterator m_descriptions;
  QStringIterator m_algorithmProperties;
  BoolIterator m_isShown;
  QStringIterator m_prefixes;
  BoolIterator m_isKey;
};

ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON
operator+(const ConstColumnIterator &lhs,
          ConstColumnIterator::difference_type n);
ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON
operator+(ConstColumnIterator::difference_type n,
          const ConstColumnIterator &rhs);
ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON
operator-(const ConstColumnIterator &lhs,
          ConstColumnIterator::difference_type n);
ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON
operator-(ConstColumnIterator::difference_type n,
          const ConstColumnIterator &rhs);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORCONSTCOLUMNITERATOR_H
