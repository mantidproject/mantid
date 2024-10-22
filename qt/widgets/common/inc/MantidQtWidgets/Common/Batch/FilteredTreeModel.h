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
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QSortFilterProxyModel>
#include <memory>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON FilteredTreeModel : public QSortFilterProxyModel {
public:
  FilteredTreeModel(RowLocationAdapter rowLocation, QObject *parent = nullptr);
  void setPredicate(std::unique_ptr<RowPredicate> predicate);
  void resetPredicate();
  bool isReset() const;
  RowLocation rowLocationAt(QModelIndex const &index) const;

protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
  std::unique_ptr<RowPredicate> m_predicate;
  RowLocationAdapter m_rowLocation;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
