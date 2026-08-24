// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <QBrush>
#include <QPalette>
#include <QTreeView>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace WorkspaceTree {
/// Item data role used to mark rows muted by the current plot output type.
auto constexpr mutedRole = Qt::UserRole + 4;
/// Background brush for rows that are not selectable for the current plot output type.
inline QBrush mutedBackgroundBrush(QPalette const &palette) { return palette.brush(QPalette::AlternateBase); }
} // namespace WorkspaceTree

/// Tree view that paints muted plotting workspace rows across all columns.
class MANTIDQT_ISISREFLECTOMETRY_DLL WorkspaceTreeView : public QTreeView {
public:
  explicit WorkspaceTreeView(QWidget *parent = nullptr);

protected:
  /// Paint muted rows as a continuous background across the full tree row.
  void drawRow(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
