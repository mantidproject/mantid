// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressModel::AlgorithmProgressModel()
    : AlgorithmObserver(),
      presenters{std::vector<AlgorithmProgressPresenterBase *>()},
      observers{std::vector<std::unique_ptr<ProgressObserver>>()} {
  // Start capturing the triggers from ALL algorithms starting
  // this allows us to attach an observer to the algorithm to track the
  // start, progress, finish and errors.
  observeStarting();
}
void AlgorithmProgressModel::addPresenter(
    const AlgorithmProgressPresenterBase *newPresenter) {
  presenters.emplace_back(newPresenter);
}

void AlgorithmProgressModel::updatePresenters() {
  for (auto &presenter : presenters) {
    presenter->update();
  }
}
void AlgorithmProgressModel::removePresenter(
    const AlgorithmProgressPresenterBase *presenter) {
  const auto it = std::find(presenters.begin(), presenters.end(), presenter);
  if (it != presenters.end()) {
    presenters.erase(it);
  }
}
void AlgorithmProgressModel::removeObserver(ProgressObserver *observer) {
  const auto it = std::find(observers.begin(), observers.end(), observer);
  if (it != observers.end()) {
    // TODO check if this is freed after the erase from the vector
    observer->stopObservingCurrent();
    observers.erase(it);
    updatePresenters();
  }
}
void AlgorithmProgressModel::updateProgress(Mantid::API::IAlgorithm_sptr alg,
                                            const double p,
                                            const std::string &msg) {
  for (auto &presenter : presenters) {
    presenter->updateProgressBar(alg, p, msg);
  }
}

void AlgorithmProgressModel::startingHandle(Mantid::API::IAlgorithm_sptr alg) {
  auto po = std::make_unique<ProgressObserver>(this, alg);
  po->observeProgress(alg);
  po->observeFinish(alg);
  po->observeError(alg);
  // give ownership to the vector, when the observer
  // is removed from it, it will be freed
  observers.emplace_back(std::move(po));
  updatePresenters();
}

std::vector<Mantid::API::IAlgorithm_sptr>
AlgorithmProgressModel::runningAlgorithms() const {
  auto vec{std::vector<Mantid::API::IAlgorithm_sptr>()};
  vec.reserve(observers.size());

  for (const auto &obs : observers) {
    vec.emplace_back(obs->currentAlgorithm());
  }

  return vec;
}

Mantid::API::IAlgorithm_sptr
AlgorithmProgressModel::latestRunningAlgorithm() const {
  return observers.back()->currentAlgorithm();
}

std::vector<RunningAlgorithmData>
AlgorithmProgressModel::runningAlgorithmsData() const {
  auto vec{std::vector<RunningAlgorithmData>()};
  vec.reserve(observers.size());

  for (const auto &obs : observers) {
    vec.emplace_back(RunningAlgorithmData{obs->name(), obs->currentAlgorithm(),
                                          obs->properties()});
  }
  return vec;
}
} // namespace MantidWidgets
} // namespace MantidQt