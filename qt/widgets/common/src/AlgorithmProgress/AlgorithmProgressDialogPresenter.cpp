// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressDialogPresenter::AlgorithmProgressDialogPresenter(
    QWidget *parent, AlgorithmProgressDialogWidget *view,
    AlgorithmProgressModel &model)
    : AlgorithmProgressPresenterBase(parent), m_view{view}, m_model{model},
      m_progressBars{RunningAlgorithms()} {
  model.setDialog(this);
}

void AlgorithmProgressDialogPresenter::algorithmStartedSlot(
    Mantid::API::AlgorithmID alg) {
  const auto algInstance =
      Mantid::API::AlgorithmManager::Instance().getAlgorithm(alg);

  if (algInstance) {
    auto treeItem =
        m_view->addAlgorithm(algInstance->name(), algInstance->getProperties());
    m_progressBars.insert(std::make_pair(alg, treeItem));
  }
}

void AlgorithmProgressDialogPresenter::updateProgressBarSlot(
    Mantid::API::AlgorithmID alg, double progress, QString message) {
  // if the algorithm isn't contained in the progress bar tree, then pretend it
  // just started
  if (m_progressBars.count(alg) == 0) {
    algorithmStartedSlot(alg);

    // the algorithm did not have an active instance, and was not created,
    // it has likely already finished, so don't do any work updating
    if (m_progressBars.count(alg) == 0) {
      return;
    }
  }
  setProgressBar(m_progressBars.at(alg).second, progress, message);
}

void AlgorithmProgressDialogPresenter::algorithmEndedSlot(
    Mantid::API::AlgorithmID alg) {
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

} // namespace MantidWidgets
} // namespace MantidQt
