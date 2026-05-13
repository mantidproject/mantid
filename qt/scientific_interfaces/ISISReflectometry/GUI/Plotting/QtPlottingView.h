// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingView.h"
#include "ui_PlottingWidget.h"
#include <QItemSelection>
#include <QStandardItemModel>

class QMouseEvent;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL QtPlottingView : public QWidget, public IPlottingView {
  Q_OBJECT
public:
  explicit QtPlottingView(QWidget *parent = nullptr);

  void subscribe(PlottingViewSubscriber *notifyee) override;
  void setOutputOptionsEnabled(bool enabled) override;
  void setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem> const &items) override;
  std::vector<std::string> selectedWorkspaces() const override;
  PlotOutputType selectedPlotOutputType() const override;

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void initLayout();
  void setOutputOptionControlsEnabled(bool enabled);
  void addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeItem const &item);
  bool isWorkspaceItem(QModelIndex const &index) const;
  bool handleWorkspaceTreeClick(QMouseEvent const &event);
  bool isAdditiveSelectionModifier(QMouseEvent const &event) const;
  bool hasSelectedAncestor(QModelIndex const &index) const;
  bool isSubtreeSelected(QModelIndex const &parentIndex) const;
  void selectSubtree(QModelIndex const &parentIndex, QItemSelectionModel::SelectionFlags selectionFlags);
  void updateChildSelection(QItemSelection const &selection, QItemSelectionModel::SelectionFlags selectionFlags);
  void updateChildSelection(QModelIndex const &parentIndex, QItemSelectionModel::SelectionFlags selectionFlags);

  Ui::PlottingWidget m_ui;
  QStandardItemModel m_workspaceModel;
  PlottingViewSubscriber *m_notifyee;
  bool m_updatingSelection;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
