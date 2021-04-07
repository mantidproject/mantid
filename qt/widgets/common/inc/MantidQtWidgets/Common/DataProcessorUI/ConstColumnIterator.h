// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/DataProcessorUI/Column.h"
#include "MantidQtWidgets/Common/DllOption.h"
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
  ConstColumnIterator(QStringIterator names, QStringIterator descriptions, QStringIterator algorithmProperties,
                      BoolIterator isShown, QStringIterator prefixes, BoolIterator isKey);

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

ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON operator+(const ConstColumnIterator &lhs,
                                                         ConstColumnIterator::difference_type n);
ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON operator+(ConstColumnIterator::difference_type n,
                                                         const ConstColumnIterator &rhs);
ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON operator-(const ConstColumnIterator &lhs,
                                                         ConstColumnIterator::difference_type n);
ConstColumnIterator EXPORT_OPT_MANTIDQT_COMMON operator-(ConstColumnIterator::difference_type n,
                                                         const ConstColumnIterator &rhs);
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt