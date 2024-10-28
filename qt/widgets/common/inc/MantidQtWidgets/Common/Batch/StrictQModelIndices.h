// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QModelIndex>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

template <typename Derived> class StrictQModelIndex {
public:
  explicit StrictQModelIndex(QModelIndex const &index);
  StrictQModelIndex() = default;

  QModelIndex untyped() const;
  int row() const;
  int column() const;
  bool isValid() const;

  Derived parent() const;
  Derived sibling(int row, int column) const;

private:
  QModelIndex m_untypedIndex;
};

template <typename Derived>
StrictQModelIndex<Derived>::StrictQModelIndex(QModelIndex const &index) : m_untypedIndex(index) {}

template <typename Derived> QModelIndex StrictQModelIndex<Derived>::untyped() const { return m_untypedIndex; }

template <typename Derived>
bool operator==(StrictQModelIndex<Derived> const &lhs, StrictQModelIndex<Derived> const &rhs) {
  return lhs.untyped() == rhs.untyped();
}

template <typename Derived> int StrictQModelIndex<Derived>::row() const { return m_untypedIndex.row(); }

template <typename Derived> int StrictQModelIndex<Derived>::column() const { return m_untypedIndex.column(); }

template <typename Derived> bool StrictQModelIndex<Derived>::isValid() const { return m_untypedIndex.isValid(); }

template <typename Derived> Derived StrictQModelIndex<Derived>::parent() const {
  return Derived(m_untypedIndex.parent());
}

template <typename Derived> Derived StrictQModelIndex<Derived>::sibling(int row, int column) const {
  return Derived(m_untypedIndex.sibling(row, column));
}

class EXPORT_OPT_MANTIDQT_COMMON QModelIndexForFilteredModel : public StrictQModelIndex<QModelIndexForFilteredModel> {
  using StrictQModelIndex<QModelIndexForFilteredModel>::StrictQModelIndex;
};

inline QModelIndexForFilteredModel fromFilteredModel(QModelIndex const &filteredModelIndex,
                                                     QAbstractItemModel const &model) {
  assertOrThrow(filteredModelIndex.model() == nullptr || filteredModelIndex.model() == &model,
                "assertFromFilteredModel: Index model assertion was not true.");
  return QModelIndexForFilteredModel(filteredModelIndex);
}

class EXPORT_OPT_MANTIDQT_COMMON QModelIndexForMainModel : public StrictQModelIndex<QModelIndexForMainModel> {
  using StrictQModelIndex<QModelIndexForMainModel>::StrictQModelIndex;
};

inline QModelIndexForMainModel fromMainModel(QModelIndex const &mainModelIndex, QAbstractItemModel const &model) {
  assertOrThrow(mainModelIndex.model() == nullptr || mainModelIndex.model() == &model,
                "assertFromMainModel: Index model assertion was not true.");
  return QModelIndexForMainModel(mainModelIndex);
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
