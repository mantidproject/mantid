// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <QColor>
#include <QTreeView>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace WorkspaceTree {
auto constexpr mutedRole = Qt::UserRole + 4;
inline QColor mutedBackgroundColour() { return QColor(238, 238, 238); }
} // namespace WorkspaceTree

class MANTIDQT_ISISREFLECTOMETRY_DLL WorkspaceTreeView : public QTreeView {
public:
  explicit WorkspaceTreeView(QWidget *parent = nullptr);

protected:
  void drawRow(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
