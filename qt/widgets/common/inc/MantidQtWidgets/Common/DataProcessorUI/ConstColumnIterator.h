#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCONSTCOLUMNITERATOR_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCONSTCOLUMNITERATOR_H
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Column.h"
#include <QString>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
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
                      QStringIterator prefixes);

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
