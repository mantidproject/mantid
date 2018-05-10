#include "MantidQtWidgets/Common/AlgorithmRunner.h"

#include <Poco/ActiveResult.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
AlgorithmRunner::AlgorithmRunner(QObject *parent)
    : QObject(parent),
      m_finishedObserver(*this,
                         &AlgorithmRunner::handleAlgorithmFinishedNotification),
      m_progressObserver(*this,
                         &AlgorithmRunner::handleAlgorithmProgressNotification),
      m_errorObserver(*this,
                      &AlgorithmRunner::handleAlgorithmErrorNotification),
      m_asyncResult(nullptr) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
AlgorithmRunner::~AlgorithmRunner() {
  if (m_asyncAlg) {
    m_asyncAlg->removeObserver(m_finishedObserver);
    m_asyncAlg->removeObserver(m_errorObserver);
    m_asyncAlg->removeObserver(m_progressObserver);
  }
  delete m_asyncResult;
}

//--------------------------------------------------------------------------------------
/** If an algorithm is already running, cancel it.
 * Does nothing if no algorithm is running. This blocks
 * for up to 1 second to wait for the algorithm to finish cancelling.
 */
void AlgorithmRunner::cancelRunningAlgorithm() {
  // Cancel any currently running algorithms
  if (m_asyncAlg) {
    if (m_asyncAlg->isRunning()) {
      m_asyncAlg->cancel();
    }
    if (m_asyncResult) {
      m_asyncResult->tryWait(1000);
      delete m_asyncResult;
      m_asyncResult = nullptr;
    }
    m_asyncAlg->removeObserver(m_finishedObserver);
    m_asyncAlg->removeObserver(m_errorObserver);
    m_asyncAlg->removeObserver(m_progressObserver);
    m_asyncAlg.reset();
  }
}

//--------------------------------------------------------------------------------------
/** Begin asynchronous execution of an algorithm and observe its execution
 *
 * @param alg :: algorithm to execute. All properties should have been set
 *properly.
 */
void AlgorithmRunner::startAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
  if (!alg)
    throw std::invalid_argument(
        "AlgorithmRunner::startAlgorithm() given a NULL Algorithm");
  if (!alg->isInitialized())
    throw std::invalid_argument(
        "AlgorithmRunner::startAlgorithm() given an uninitialized Algorithm");

  cancelRunningAlgorithm();

  // Observe the algorithm
  alg->addObserver(m_finishedObserver);
  alg->addObserver(m_errorObserver);
  alg->addObserver(m_progressObserver);
  // Start asynchronous execution
  m_asyncAlg = alg;
  m_asyncResult = new Poco::ActiveResult<bool>(m_asyncAlg->executeAsync());
}

/// Get back a pointer to the running algorithm
Mantid::API::IAlgorithm_sptr AlgorithmRunner::getAlgorithm() const {
  return m_asyncAlg;
}

//--------------------------------------------------------------------------------------
/** Observer called when the asynchronous algorithm has completed.
 *
 * Emits a signal for the GUI widget
 *
 * This is called in a separate (non-GUI) thread and so
 * CANNOT directly change the gui.
 *
 * @param pNf :: finished notification object.
 */
void AlgorithmRunner::handleAlgorithmFinishedNotification(
    const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf) {
  UNUSED_ARG(pNf);
  emit algorithmComplete(false);
}

//--------------------------------------------------------------------------------------
/** Observer called when the async algorithm has progress to report
 *
 * @param pNf :: notification object
 */
void AlgorithmRunner::handleAlgorithmProgressNotification(
    const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf) {
  emit algorithmProgress(pNf->progress, pNf->message);
}

//--------------------------------------------------------------------------------------
/** Observer called when the async algorithm has encountered an error.
 * Emits a signal for the GUI widget
 *
 * @param pNf :: notification object
 */
void AlgorithmRunner::handleAlgorithmErrorNotification(
    const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf) {
  UNUSED_ARG(pNf);
  emit algorithmComplete(true);
}

} // namespace API
} // namespace MantidQt
