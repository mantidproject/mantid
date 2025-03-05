// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QTreeWidgetItem>
#include <unordered_map>

/**
 * The AlgorithmProgressDialogPresenter keeps track of the running algorithms
 * and displays a progress bar for them, and a property list.
 */
namespace MantidQt {
namespace MantidWidgets {
class AlgorithmProgressModel;
class IAlgorithmProgressDialogWidget;

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmProgressDialogPresenter : public AlgorithmProgressPresenterBase {
  Q_OBJECT
  using RunningAlgorithms = std::unordered_map<Mantid::API::AlgorithmID, std::pair<QTreeWidgetItem *, QProgressBar *>>;

public:
  AlgorithmProgressDialogPresenter(QWidget *parent, IAlgorithmProgressDialogWidget *view,
                                   AlgorithmProgressModel &model);

  void algorithmStartedSlot(Mantid::API::AlgorithmID) override;
  void updateProgressBarSlot(Mantid::API::AlgorithmID, const double progress, const QString &message,
                             const double estimatedTime, const int progressPrecision) override;
  void algorithmEndedSlot(Mantid::API::AlgorithmID) override;
  size_t getNumberTrackedAlgorithms();

private:
  IAlgorithmProgressDialogWidget *m_view;
  /// Reference to the model of the main window progress bar
  AlgorithmProgressModel &m_model;
  /// Container for all the progress bars that are currently being displayed
  /// This container does NOT own any of the progress bars
  RunningAlgorithms m_progressBars;
};
} // namespace MantidWidgets
} // namespace MantidQt
