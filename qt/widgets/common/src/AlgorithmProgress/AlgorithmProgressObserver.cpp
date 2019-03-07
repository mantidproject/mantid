// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressObserver.h"
#include "MantidAPI/IAlgorithm.h"

namespace MantidQt {
namespace MantidWidgets {

ProgressObserver::ProgressObserver(AlgorithmProgressModel *model,
                                   Mantid::API::IAlgorithm_sptr alg)
    : m_model(model), m_alg(alg) {}

void ProgressObserver::finishHandle(const Mantid::API::IAlgorithm *alg) {
  m_model->removeObserver(this);
}

void ProgressObserver::progressHandle(const Mantid::API::IAlgorithm *alg,
                                      const double p, const std::string &msg) {
  progress = p;
  message = msg;

  m_model->updateProgress(m_alg, progress, message);
}

void ProgressObserver::errorHandle(const Mantid::API::IAlgorithm *alg,
                                   const std::string &what) {
  m_model->removeObserver(this);
}

void ProgressObserver::stopObservingCurrent() { this->stopObserving(m_alg); }

Mantid::API::IAlgorithm_sptr ProgressObserver::currentAlgorithm() const {
  return m_alg;
}

} // namespace MantidWidgets
} // namespace MantidQt