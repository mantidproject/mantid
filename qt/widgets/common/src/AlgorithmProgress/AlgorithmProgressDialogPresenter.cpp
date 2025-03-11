// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressDialogWidget.h"

namespace MantidQt::MantidWidgets {

AlgorithmProgressDialogPresenter::AlgorithmProgressDialogPresenter(QWidget *parent,
                                                                   IAlgorithmProgressDialogWidget *view,
                                                                   AlgorithmProgressModel &model)
    : AlgorithmProgressPresenterBase(parent), m_view{view}, m_model{model}, m_progressBars{RunningAlgorithms()} {
  model.setDialog(this);

  // Intital setup of any running algorithms
  auto runningAlgorithms = Mantid::API::AlgorithmManager::Instance().runningInstances();

  for (const auto &alg : runningAlgorithms) {
    if ((alg) && (alg->isRunning())) {
      if (m_progressBars.count(alg->getAlgorithmID()) == 0) {
        algorithmStartedSlot(alg->getAlgorithmID());
      }
    }
  }
}

/// This slot is triggered whenever an algorithm has started executing
/// @param alg The ID of the algorithm that has started executing
void AlgorithmProgressDialogPresenter::algorithmStartedSlot(Mantid::API::AlgorithmID alg) {

  const auto algInstance = Mantid::API::AlgorithmManager::Instance().getAlgorithm(alg);

  // There is a chance that the algorithm has already finished
  // -> if the updateProgressBarSlot triggers this algorithmStartedSlot, but the
  // original algorithm has already finished, before we got a shared pointer.
  // This ensures that the tracking only looks after an algorithm that has not
  // finished
  if (m_progressBars.find(alg) == m_progressBars.end()) {
    if (algInstance) {
      auto treeItem = m_view->addAlgorithm(algInstance);
      m_progressBars.insert(std::make_pair(alg, treeItem));
    }
  }
}
/// This slot is triggered whenever an algorithm reports progress.
/// If the algorithm is not being tracked (e.g. the Details window is
/// opened _AFTER_ the algorithm has started) then the started slot is triggered
/// to add an active progress bar for it.
/// @param alg The ID of the algorithm that is reporting progress
/// @param progress The progress that the algorithm has reported
/// @param message The message that the algorithm has reported. It can be
/// emitted from another thread, so a copy of the message is forced
/// @param estimatedTime :: estimated time to completion in seconds
/// @param progressPrecision :: number of digits after the decimal
void AlgorithmProgressDialogPresenter::updateProgressBarSlot(Mantid::API::AlgorithmID alg, const double progress,
                                                             const QString &message, const double estimatedTime,
                                                             const int progressPrecision) {
  // if the algorithm isn't contained in the progress bar tree, then pretend it
  // just started
  if (m_progressBars.count(alg) == 0) {
    algorithmStartedSlot(alg);

    // the algorithm did not have an active instance, and was not created,
    // it has already finished, so don't do any work updating
    if (m_progressBars.count(alg) == 0) {
      return;
    }
  }
  setProgressBar(m_progressBars.at(alg).second, progress, message, estimatedTime, progressPrecision);
}

/// This slot is triggered whenever an algorithms ends. If the algorithm is not
/// being tracked, there will not be an active progress bar for it, in which
/// case no work is done
/// @param alg The ID of the algorithm that is ending
void AlgorithmProgressDialogPresenter::algorithmEndedSlot(Mantid::API::AlgorithmID alg) {
  // if the algorithm has an active widget, then delete it
  // if the count is actually 0, then the algorithm ended
  // before a widget was ever made or displayed, so we avoid doing any work
  if (m_progressBars.count(alg) != 0) {
    // keep the pointer to the QTreeItemWidget
    const auto item = m_progressBars.at(alg).first;
    m_progressBars.erase(alg);
    // this removes the item from the tree widget, and deletes all its children
    delete item;
  }
}
size_t AlgorithmProgressDialogPresenter::getNumberTrackedAlgorithms() { return m_progressBars.size(); }

} // namespace MantidQt::MantidWidgets
