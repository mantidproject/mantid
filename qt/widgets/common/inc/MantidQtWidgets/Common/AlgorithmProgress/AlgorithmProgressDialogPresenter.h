// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSDIALOGPRESENTER_H
#define ALGORITHMPROGRESSDIALOGPRESENTER_H

#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"

#include <QTreeWidgetItem>
#include <unordered_map>

/**
 * The AlgorithmProgressDialogPresenter keeps track of the running algorithms
 * and displays a progress bar for them, and a property list.
 */
namespace MantidQt {
namespace MantidWidgets {
class AlgorithmProgressModel;
class AlgorithmProgressDialogWidget;

class AlgorithmProgressDialogPresenter : public AlgorithmProgressPresenterBase {
  Q_OBJECT
  using RunningAlgorithms =
      std::unordered_map<Mantid::API::AlgorithmID,
                         std::pair<QTreeWidgetItem *, QProgressBar *>>;

public:
  AlgorithmProgressDialogPresenter(QWidget *parent,
                                   AlgorithmProgressDialogWidget *view,
                                   AlgorithmProgressModel &model);

  void algorithmStartedSlot(Mantid::API::AlgorithmID /*unused*/) override;
  void updateProgressBarSlot(Mantid::API::AlgorithmID /*unused*/, double /*unused*/,
                             QString /*unused*/) override;
  void algorithmEndedSlot(Mantid::API::AlgorithmID /*unused*/) override;

private:
  AlgorithmProgressDialogWidget *m_view;
  /// Reference to the model of the main window progress bar
  AlgorithmProgressModel &m_model;
  /// Container for all the progress bars that are currently being displayed
  /// This container does NOT own any of the progress bars
  RunningAlgorithms m_progressBars;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGPRESENTER_H