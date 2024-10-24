// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//----------------------------------
// Includes
//----------------------------------
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressWidget.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QHBoxLayout>
#include <QPointer>
#include <QPushButton>
#include <QWidget>
#include <memory>

class QProgressBar;
class QString;

/** The AlgorithmProgressWidget shows the main progress bar always visible on
 * the Workbench.
 *
 * This class is also the creator/owner of the Details window, which can show
 * multiple progress bars for algorithms running simultaneously (e.g. from 2
 * code editors).
 */

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmProgressWidget : public QWidget, public IAlgorithmProgressWidget {
  Q_OBJECT
public:
  AlgorithmProgressWidget(QWidget *parent = nullptr);

  /// Setup the view for whenever an algorithm has started.
  void algorithmStarted() override;
  /// Setup the view for whenever an algorithm has ended.
  void algorithmEnded() override;
  /// Enable or disable the processing of updates to the algorithm progress
  void blockUpdates(bool block = true);
  /// Update the progress bar
  void updateProgress(const double progress, const QString &message, const double estimatedTime,
                      const int progressPrecision) override;

public slots:
  void showDetailsDialog() override;

private:
  // Widgets are managed by Qt. This class owns them, and Qt will delete them on
  // close.
  /// Progress bar shown on the workbench
  QProgressBar *const m_progressBar;
  /// Layout that contains all the widget for displaying
  QHBoxLayout *const m_layout;
  /// Button to display the Details window
  QPushButton *const m_detailsButton;
  /// Pointer to the details dialog. Will be set to nullptr by QPointer when the
  /// dialog closes
  QPointer<AlgorithmProgressDialogWidget> m_details;
  /// The presenter of the ProgressWidget
  std::unique_ptr<AlgorithmProgressPresenter> m_presenter;
};

} // namespace MantidWidgets
} // namespace MantidQt
