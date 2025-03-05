// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"

namespace MantidQt::MantidWidgets {
AlgorithmProgressPresenter::AlgorithmProgressPresenter(QWidget *parent, IAlgorithmProgressWidget *view)
    : AlgorithmProgressPresenterBase(parent), m_model{AlgorithmProgressModel(this)}, m_algorithm(nullptr), m_view(view),
      m_timer() {}

void AlgorithmProgressPresenter::algorithmStartedSlot(Mantid::API::AlgorithmID alg) {
  // only allow the tracking of one algorithm at a time
  // this makes the progress bar stutter less and it looks better overall
  if (!m_algorithm) {
    m_algorithm = alg;
    m_view->algorithmStarted();
  }
}

void AlgorithmProgressPresenter::algorithmEndedSlot(Mantid::API::AlgorithmID alg) {
  if (alg == this->m_algorithm) {
    m_algorithm = nullptr;
    m_view->algorithmEnded();
  }
}

/// This slot is triggered whenever an algorithm reports progress.
/// If the algorithm is not being tracked, then the progress bar isn't changed.
/// This can happen whenever there are multiple algorithms running
/// simultaneously - the progress bar will only show the progress of the first
/// one that started. If many algorithms are starting, this prevents having a
/// very jittery progress bar, that never completely fills up
/// @param algorithm The ID of the algorithm that is reporting progress
/// @param progress The progress that the algorithm has reported
/// @param message The message that the algorithm has reported. It can be
/// emitted from another thread, so a copy of the message is forced
/// @param estimatedTime :: estimated time to completion in seconds
/// @param progressPrecision :: number of digits after the decimal
void AlgorithmProgressPresenter::updateProgressBarSlot(Mantid::API::AlgorithmID algorithm, const double progress,
                                                       const QString &message, const double estimatedTime,
                                                       const int progressPrecision) {
  if (algorithm == this->m_algorithm) {
    // this needs to be a call to the view
    // so that it can be mocked out for testing
    constexpr float maxRefreshInterval{0.1f};
    float timeInterval = m_timer.elapsed_no_reset();
    if (timeInterval > maxRefreshInterval) {
      m_timer.reset();
      m_view->updateProgress(progress, message, estimatedTime, progressPrecision);
    }
  }
}

} // namespace MantidQt::MantidWidgets
