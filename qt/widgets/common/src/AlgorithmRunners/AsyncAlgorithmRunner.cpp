// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmRunners/AsyncAlgorithmRunner.h"
#include "MantidQtWidgets/Common/AlgorithmRunners/IAsyncAlgorithmSubscriber.h"

#include <Poco/ActiveResult.h>

using namespace Mantid::API;

namespace MantidQt::API {

AsyncAlgorithmRunner::AsyncAlgorithmRunner()
    : m_finishedObserver(*this, &AsyncAlgorithmRunner::onAlgorithmFinished),
      m_progressObserver(*this, &AsyncAlgorithmRunner::onAlgorithmProgress),
      m_errorObserver(*this, &AsyncAlgorithmRunner::onAlgorithmError), m_algorithm() {}

AsyncAlgorithmRunner::~AsyncAlgorithmRunner() {
  if (m_algorithm) {
    m_algorithm->removeObserver(m_finishedObserver);
    m_algorithm->removeObserver(m_errorObserver);
    m_algorithm->removeObserver(m_progressObserver);
  }
}

void AsyncAlgorithmRunner::subscribe(IAsyncAlgorithmSubscriber *subscriber) { m_subscriber = subscriber; }

void AsyncAlgorithmRunner::cancelRunningAlgorithm() {
  if (m_algorithm) {
    if (m_algorithm->isRunning()) {
      m_algorithm->cancel();
    }
    m_algorithm->removeObserver(m_finishedObserver);
    m_algorithm->removeObserver(m_errorObserver);
    m_algorithm->removeObserver(m_progressObserver);
    m_algorithm.reset();
  }
}

void AsyncAlgorithmRunner::startAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
  if (!alg) {
    throw std::invalid_argument("AsyncAlgorithmRunner: The provided algorithm is null.");
  }
  if (!alg->isInitialized()) {
    throw std::invalid_argument("AsyncAlgorithmRunner: The provided algorithm is uninitialized.");
  }

  cancelRunningAlgorithm();

  m_algorithm = alg;
  m_algorithm->addObserver(m_finishedObserver);
  m_algorithm->addObserver(m_errorObserver);
  m_algorithm->addObserver(m_progressObserver);
  m_algorithm->executeAsync();
}

Mantid::API::IAlgorithm_sptr AsyncAlgorithmRunner::getAlgorithm() const { return m_algorithm; }

void AsyncAlgorithmRunner::onAlgorithmFinished(const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf) {
  m_subscriber->notifyAlgorithmFinished(m_algorithm->name(), false);
}

void AsyncAlgorithmRunner::onAlgorithmProgress(const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf) {
  m_subscriber->notifyAlgorithmProgress(pNf->progress, pNf->message);
}

void AsyncAlgorithmRunner::onAlgorithmError(const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf) {
  m_subscriber->notifyAlgorithmFinished(m_algorithm->name(), true);
}

} // namespace MantidQt::API
