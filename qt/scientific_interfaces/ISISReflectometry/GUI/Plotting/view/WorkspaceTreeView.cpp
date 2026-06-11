// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "WorkspaceTreeView.h"

#include <QPainter>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

WorkspaceTreeView::WorkspaceTreeView(QWidget *parent) : QTreeView(parent) {}

void WorkspaceTreeView::drawRow(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const {
  if (index.data(WorkspaceTree::mutedRole).toBool()) {
    painter->fillRect(option.rect, WorkspaceTree::mutedBackgroundColour());
  }

  QTreeView::drawRow(painter, option, index);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
