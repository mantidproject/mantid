// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressWidget.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QWidget>

/**
 * The AlgorithmProgressPresenter is the presenter for the progress bar always
 * visible on the Workbench. It will update the progress bar with the first
 * algorithm that has been run. If there are two or more running simultaneously,
 * only the first one's progress will be displayed.
 *
 * When the algorithm finishes, the ID is cleared, and the next algorithm that
 * starts will be tracked.
 *
 * Tracking only 1 algorithm at a time was done to allow the progress bar to
 * completely fill up, rather than be constantly reset by newly started
 * algorithms.
 *
 * More than one progress bar is handled by the AlgorithmProgressDialogWidget.
 */
namespace MantidQt {
namespace MantidWidgets {
class IAlgorithmProgressWidget;

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmProgressPresenter : public AlgorithmProgressPresenterBase {
  Q_OBJECT

public:
  AlgorithmProgressPresenter(QWidget *parent, IAlgorithmProgressWidget *);

  void algorithmStartedSlot(Mantid::API::AlgorithmID) override;
  void updateProgressBarSlot(Mantid::API::AlgorithmID, const double, const QString &, const double, const int) override;
  void algorithmEndedSlot(Mantid::API::AlgorithmID) override;

  AlgorithmProgressModel &model() { return m_model; }

private:
  /// The model which observes events happening to the algorithms
  AlgorithmProgressModel m_model;
  /// The algorithm for which a progress bar is currently being controlled
  Mantid::API::AlgorithmID m_algorithm;
  /// The view that contains the progress widget.
  /// The creator of the view also owns the view (Python), not this presenter.
  IAlgorithmProgressWidget *m_view;
  /// Atimer to meaure how long since the last progress report
  Mantid::Kernel::Timer m_timer;
};
} // namespace MantidWidgets
} // namespace MantidQt
