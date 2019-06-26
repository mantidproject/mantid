// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_ALGORITHMPROGRESSDIALOGWIDGET_H
#define MANTID_MANTIDWIDGETS_ALGORITHMPROGRESSDIALOGWIDGET_H

#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressDialogWidget.h"

#include <QDialog>
#include <QPushButton>

#include <memory>
#include <utility>

class QProgressBar;

/**
 * The AlgorithmProgressDialogWidget displays multiple progress bars for
 * algorithms running simultaneously. This widget shares uses the model from the
 * main Workbench progress bar (AlgorithmProgressWidget).
 */
namespace MantidQt {
namespace MantidWidgets {
class AlgorithmProgressDialogWidget : public QDialog,
                                      public IAlgorithmProgressDialogWidget {
  Q_OBJECT
public:
  AlgorithmProgressDialogWidget(QWidget *parent, AlgorithmProgressModel &model);

  /// Adds an algorithm to the dialog. Returns the item in the tree widget, and
  /// the progress bar within it
  std::pair<QTreeWidgetItem *, QProgressBar *>
  addAlgorithm(Mantid::API::IAlgorithm_sptr alg) override;

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  std::unique_ptr<AlgorithmProgressDialogPresenter> m_presenter;
  /// Owned by this dialog, will be deleted on close
  QTreeWidget *m_tree;
};

/*
 * The AlgorithmProgressDialogWidgetCancelButton handles the Cancel buttons
 * displayed in the dialog. It keeps a copy of the shared pointer to the
 * algorithm to ensure that it can always be closed when the user clicks the
 * button
 */
class AlgorithmProgressDialogWidgetCancelButton : public QPushButton {
  Q_OBJECT
public:
  AlgorithmProgressDialogWidgetCancelButton(Mantid::API::IAlgorithm_sptr alg,
                                            QWidget *parent = 0)
      : QPushButton("Cancel", parent), m_alg(std::move(alg)) {
    connect(this, &QPushButton::clicked, this,
            &AlgorithmProgressDialogWidgetCancelButton::clickedWithAlgSlot);
  }

signals:
  void clickedWithAlg(Mantid::API::IAlgorithm_sptr alg);

private slots:
  void clickedWithAlgSlot() { m_alg->cancel(); }

private:
  Mantid::API::IAlgorithm_sptr m_alg;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGWIDGET_H