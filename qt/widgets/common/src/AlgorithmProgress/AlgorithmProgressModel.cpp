// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressModel::AlgorithmProgressModel(
    AlgorithmProgressPresenter *presenter)
    : AlgorithmObserver(), m_dialogPresenter{nullptr}, m_mainWindowPresenter{
                                                           presenter} {
  // Start capturing the triggers from ALL algorithms starting
  // this allows us to attach an observer to the algorithm to track the
  // start, progress, finish and errors.
  observeStarting();
}
inline void
AlgorithmProgressModel::setDialog(AlgorithmProgressDialogPresenter *presenter) {
  this->m_dialogPresenter = presenter;
}
void AlgorithmProgressModel::startingHandle(Mantid::API::IAlgorithm_sptr alg) {
  observeStart(alg);
  observeProgress(alg);
  observeFinish(alg);
  observeError(alg);
}

void AlgorithmProgressModel::startHandle(const Mantid::API::IAlgorithm *alg) {
  m_mainWindowPresenter->algorithmStarted(alg->getAlgorithmID());

  if (m_dialogPresenter) {
    m_dialogPresenter->algorithmStarted(alg->getAlgorithmID());
  }
}

void AlgorithmProgressModel::progressHandle(const Mantid::API::IAlgorithm *alg,
                                            const double progress,
                                            const std::string &message) {
  m_mainWindowPresenter->updateProgressBar(alg->getAlgorithmID(), progress,
                                           message);
  if (m_dialogPresenter) {
    m_dialogPresenter->updateProgressBar(alg->getAlgorithmID(), progress,
                                         message);
  }
}

void AlgorithmProgressModel::finishHandle(const Mantid::API::IAlgorithm *alg) {
  this->removeFrom(alg);
}

void AlgorithmProgressModel::errorHandle(const Mantid::API::IAlgorithm *alg,
                                         const std::string &what) {
  this->removeFrom(alg);
}

void AlgorithmProgressModel::removeFrom(const Mantid::API::IAlgorithm *alg) {
  this->stopObserving(alg);
  m_mainWindowPresenter->algorithmEnded(nullptr);
  if (m_dialogPresenter) {
    m_dialogPresenter->algorithmEnded(alg->getAlgorithmID());
  }
}
} // namespace MantidWidgets
} // namespace MantidQt