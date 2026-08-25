// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingWorkspaceTreeView.h"

#include <QPainter>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

QtPlottingWorkspaceTreeView::QtPlottingWorkspaceTreeView(QWidget *parent) : QTreeView(parent) {}

void QtPlottingWorkspaceTreeView::drawRow(QPainter *painter, QStyleOptionViewItem const &option,
                                          QModelIndex const &index) const {
  if (index.data(PlottingWorkspaceTreeView::mutedRole).toBool()) {
    painter->fillRect(option.rect, PlottingWorkspaceTreeView::mutedBackgroundBrush(palette()));
  }

  QTreeView::drawRow(painter, option, index);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
