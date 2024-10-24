// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"

namespace MantidQt::MantidWidgets {

AlgorithmProgressModel::AlgorithmProgressModel(AlgorithmProgressPresenter *presenter)
    : AlgorithmObserver(), m_dialogPresenter{nullptr}, m_mainWindowPresenter{presenter} {

  // Start capturing the triggers from ALL algorithms starting
  // this allows us to attach an observer to the algorithm to track the
  // start, progress, finish and errors.
  observeStarting();
}

AlgorithmProgressModel::~AlgorithmProgressModel() { this->stopObservingManager(); }

void AlgorithmProgressModel::setDialog(AlgorithmProgressDialogPresenter *presenter) {
  this->m_dialogPresenter = presenter;
}

/// This handle observes all starting algorithms. When an algorithm is starting
/// it will attach the model as an observer for further events.
/// @param alg The algorithm that is starting
void AlgorithmProgressModel::startingHandle(Mantid::API::IAlgorithm_sptr alg) {
  observeStart(alg);
  observeProgress(alg);
  observeFinish(alg);
  observeError(alg);
}
/// This handle is triggered when the algorithm is executed. It is used to
/// notify the necessary presenters that there is a new algorithm for which
/// progress has to be displayed
/// @param alg The algorithm that is executing
void AlgorithmProgressModel::startHandle(const Mantid::API::IAlgorithm *alg) {
  m_mainWindowPresenter->algorithmStarted(alg->getAlgorithmID());

  if (m_dialogPresenter) {
    m_dialogPresenter->algorithmStarted(alg->getAlgorithmID());
  }
}

/// This handle is triggered whenever the algorithm reports progress.
/// @param alg The algorithm that has reported progress
/// @param progress The progress that the algorithm is currently at
/// @param message The message that the progress bar should display
/// @param estimatedTime :: estimated time to completion in seconds
/// @param progressPrecision :: number of digits after the decimal
void AlgorithmProgressModel::progressHandle(const Mantid::API::IAlgorithm *alg, const double progress,
                                            const std::string &message, const double estimatedTime,
                                            const int progressPrecision) {
  m_mainWindowPresenter->updateProgressBar(alg->getAlgorithmID(), progress, message, estimatedTime, progressPrecision);
  if (m_dialogPresenter) {
    m_dialogPresenter->updateProgressBar(alg->getAlgorithmID(), progress, message, estimatedTime, progressPrecision);
  }
}
/// This handle is triggered when the algorithm finishes. It will notify
/// presenters that there is not going to be any more progress for this
/// algorithm
/// @param alg The algorithm that has finished
void AlgorithmProgressModel::finishHandle(const Mantid::API::IAlgorithm *alg) { this->removeFrom(alg); }
/// This handle is triggered when the algorithm encounters an error. It does the
/// same thing as the algorithm finishing
/// @param alg The algorithm that has finished
/// @param what Unused error message
void AlgorithmProgressModel::errorHandle(const Mantid::API::IAlgorithm *alg, const std::string &what) {
  UNUSED_ARG(what);
  this->removeFrom(alg);
}

/// Remove the algorithm from presenters
/// @param alg The algorithm that has to be removed from the presenters
void AlgorithmProgressModel::removeFrom(const Mantid::API::IAlgorithm *alg) {
  this->stopObserving(alg);
  m_mainWindowPresenter->algorithmEnded(alg->getAlgorithmID());
  if (m_dialogPresenter) {
    m_dialogPresenter->algorithmEnded(alg->getAlgorithmID());
  }
}
} // namespace MantidQt::MantidWidgets
