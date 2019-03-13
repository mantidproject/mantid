// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSDIALOGWIDGET_H
#define ALGORITHMPROGRESSDIALOGWIDGET_H

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"

#include <QDialog>

#include <memory>

class QProgressBar;

/**
 * The AlgorithmProgressDialogWidget displays multiple progress bars for
 * algorithms running simultaneously. This widget shares uses the model from the
 * main Workbench progress bar (AlgorithmProgressWidget).
 */
namespace MantidQt {
namespace MantidWidgets {
class AlgorithmProgressDialogWidget : public QDialog {
  Q_OBJECT
public:
  AlgorithmProgressDialogWidget(QWidget *parent, AlgorithmProgressModel &model);

  /// Adds an algorithm to the dialog. Returns the item in the tree widget, and
  /// the progress bar within it
  std::pair<QTreeWidgetItem *, QProgressBar *>
  addAlgorithm(std::string, std::vector<Mantid::Kernel::Property *>);

private:
  std::unique_ptr<AlgorithmProgressDialogPresenter> m_presenter;
  /// Owned by this dialog, will be deleted on close
  QTreeWidget *m_tree;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGWIDGET_H