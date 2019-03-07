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
    AlgorithmProgressWidget *view)
    : view(view), model{AlgorithmProgressModel()} {}

void AlgorithmProgressPresenter::setCurrentAlgorithm() {
  algorithm = model.latestRunningAlgorithm();
}

void AlgorithmProgressPresenter::updateProgressBar(Mantid::API::IAlgorithm_sptr alg,
                                                   const double progress,
                                                   const std::string &msg) {
  if (alg == this->algorithm) {
    auto message = std::string{""};
    if (!msg.empty()) {
      message = msg;
    }

    emit progressBarNeedsUpdating(view->pb, progress, msg);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
