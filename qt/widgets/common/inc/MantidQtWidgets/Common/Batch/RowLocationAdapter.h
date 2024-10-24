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
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QStandardItemModel>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON RowLocationAdapter {
public:
  RowLocationAdapter(QStandardItemModel const &model);

  RowLocation atIndex(QModelIndexForMainModel const &index) const;
  boost::optional<QModelIndexForMainModel> indexIfExistsAt(RowLocation const &location, int column = 0) const;
  QModelIndexForMainModel indexAt(RowLocation const &location, int column = 0) const;

private:
  QModelIndex walkFromRootToParentIndexOf(RowLocation const &location) const;
  QStandardItemModel const &m_model;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
