// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"

namespace MantidQt {
namespace MantidWidgets {
AlgorithmProgressPresenter::AlgorithmProgressPresenter(
    QWidget *parent, AlgorithmProgressWidget *view)
    : AlgorithmProgressPresenterBase(parent), m_model{AlgorithmProgressModel(
                                                  this)},
      m_algorithm(nullptr), m_view(view) {}

void AlgorithmProgressPresenter::algorithmStartedSlot(
    Mantid::API::AlgorithmID alg) {
  // only allow the tracking of one algorithm at a time
  // this makes the progress bar stutter less and it looks better overall
  if (!m_algorithm) {
    m_algorithm = alg;
    m_view->algorithmStarted();
  }
}

void AlgorithmProgressPresenter::algorithmEndedSlot(
    Mantid::API::AlgorithmID alg) {
  m_algorithm = alg;
  m_view->algorithmEnded();
}

void AlgorithmProgressPresenter::updateProgressBarSlot(
    Mantid::API::AlgorithmID algorithm, const double progress,
    QString message) {
  if (algorithm == this->m_algorithm) {
    setProgressBar(m_view->progressBar(), progress, message);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
