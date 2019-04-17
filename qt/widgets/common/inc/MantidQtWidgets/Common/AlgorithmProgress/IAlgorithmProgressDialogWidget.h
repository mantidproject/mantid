// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_IALGORITHMPROGRESSDIALOGWIDGET_H
#define MANTID_MANTIDWIDGETS_IALGORITHMPROGRESSDIALOGWIDGET_H

#include "MantidAPI/Algorithm.h"

#include <QDialog>
#include <QPushButton>

#include <memory>
#include <utility>

class QProgressBar;
class QTreeWidgetItem;

/**
 * The AlgorithmProgressDialogWidget displays multiple progress bars for
 * algorithms running simultaneously. This widget shares uses the model from the
 * main Workbench progress bar (AlgorithmProgressWidget).
 */
namespace MantidQt {
namespace MantidWidgets {
class IAlgorithmProgressDialogWidget {
public:
  /// Adds an algorithm to the dialog. Returns the item in the tree widget, and
  /// the progress bar within it
  virtual std::pair<QTreeWidgetItem *, QProgressBar *>
  addAlgorithm(Mantid::API::IAlgorithm_sptr alg) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // IALGORITHMPROGRESSDIALOGWIDGET_H