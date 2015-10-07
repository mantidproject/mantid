//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/AlgorithmManager.h"

namespace Mantid {
namespace API {

/// Default constructor. Notification handlers are not connected to any
/// algorithm
AlgorithmObserver::AlgorithmObserver()
    : m_progressObserver(*this, &AlgorithmObserver::_progressHandle),
      m_startObserver(*this, &AlgorithmObserver::_startHandle),
      m_finishObserver(*this, &AlgorithmObserver::_finishHandle),
      m_errorObserver(*this, &AlgorithmObserver::_errorHandle),
      m_startingObserver(*this, &AlgorithmObserver::_startingHandle) {}

/**   Constructs AlgorithmObserver and connects all its handlers to algorithm
   alg.
      @param alg :: Algorithm to be observed
  */
AlgorithmObserver::AlgorithmObserver(IAlgorithm_const_sptr alg)
    : m_progressObserver(*this, &AlgorithmObserver::_progressHandle),
      m_startObserver(*this, &AlgorithmObserver::_startHandle),
      m_finishObserver(*this, &AlgorithmObserver::_finishHandle),
      m_errorObserver(*this, &AlgorithmObserver::_errorHandle),
      m_startingObserver(*this, &AlgorithmObserver::_startingHandle) {
  observeAll(alg);
}

/// Virtual destructor
AlgorithmObserver::~AlgorithmObserver() {}

/**   Connect to algorithm alg and observe all its notifications
      @param alg :: Algorithm to be observed
*/
void AlgorithmObserver::observeAll(IAlgorithm_const_sptr alg) {
  alg->addObserver(m_progressObserver);
  alg->addObserver(m_startObserver);
  alg->addObserver(m_finishObserver);
  alg->addObserver(m_errorObserver);
}

/**   Connect to algorithm alg and observe its progress notification
      @param alg :: Algorithm to be observed
*/
void AlgorithmObserver::observeProgress(IAlgorithm_const_sptr alg) {
  alg->addObserver(m_progressObserver);
}

/**   Connect to AlgorithmManager and observe its starting notifications
*/
void AlgorithmObserver::observeStarting() {
  AlgorithmManager::Instance().notificationCenter.addObserver(
      m_startingObserver);
}

/**   Connect to algorithm alg and observe its start notification
      @param alg :: Algorithm to be observed
*/
void AlgorithmObserver::observeStart(IAlgorithm_const_sptr alg) {
  alg->addObserver(m_startObserver);
}

/**   Connect to algorithm alg and observe its finish notification
      @param alg :: Algorithm to be observed
*/
void AlgorithmObserver::observeFinish(IAlgorithm_const_sptr alg) {
  alg->addObserver(m_finishObserver);
}

/**   Connect to algorithm alg and observe its error notification
      @param alg :: Algorithm to be observed
*/
void AlgorithmObserver::observeError(IAlgorithm_const_sptr alg) {
  alg->addObserver(m_errorObserver);
}

/**   Disconnect from algorithm alg. Should be called in the destructor of
   inherited classes.
      @param alg :: Algorithm to be disconnected
*/
void AlgorithmObserver::stopObserving(IAlgorithm_const_sptr alg) {
  alg->removeObserver(m_progressObserver);
  alg->removeObserver(m_startObserver);
  alg->removeObserver(m_finishObserver);
  alg->removeObserver(m_errorObserver);
}

/**
 * Disconnect from the algorithm manager.
 */
void AlgorithmObserver::stopObservingManager() {
  AlgorithmManager::Instance().notificationCenter.removeObserver(
      m_startingObserver);
}

/// @cond Doxygen cannot handle the macro around the argument name

/** Handler of the progress notifications. Must be overriden in inherited
   classes.
    The default handler is provided (doing nothing).
    @param alg :: Pointer to the algorithm sending the notification. Note that
   this can
    point to a different object than the argument of a observeZZZ(...) method,
   e.g.
    an observer can be connected to an AlgorithmProxy instance and receive
   notifications from
    the corresponding Algorithm type object.
    @param p :: Progress reported by the algorithm, 0 <= p <= 1
    @param msg :: Optional message string sent by the algorithm
*/
void AlgorithmObserver::progressHandle(const IAlgorithm *alg, double p,
                                       const std::string &msg) {
  UNUSED_ARG(alg)
  UNUSED_ARG(p)
  UNUSED_ARG(msg)
}

/** Handler of the start notifications. Must be overriden in inherited classes.
    The default handler is provided (doing nothing).
    @param alg :: Shared Pointer to the algorithm sending the notification.
*/
void AlgorithmObserver::startingHandle(IAlgorithm_sptr alg) { UNUSED_ARG(alg) }

/** Handler of the start notifications. Must be overriden in inherited classes.
    The default handler is provided (doing nothing).
    @param alg :: Pointer to the algorithm sending the notification. Note that
   this can
    point to a different object than the argument of a observeZZZ(...) method,
   e.g.
    an observer can be connected to an AlgorithmProxy instance and receive
   notifications from
    the corresponding Algorithm type object.
*/
void AlgorithmObserver::startHandle(const IAlgorithm *alg) { UNUSED_ARG(alg) }

/** Handler of the finish notifications. Must be overriden in inherited classes.
     The default handler is provided (doing nothing).
     @param alg :: Pointer to the algorithm sending the notification. Note that
   this can
     point to a different object than the argument of a observeZZZ(...) method,
   e.g.
     an observer can be connected to an AlgorithmProxy instance and receive
   notifications from
     the corresponding Algorithm type object.
 */
void AlgorithmObserver::finishHandle(const IAlgorithm *alg) { UNUSED_ARG(alg) }

/** Handler of the error notifications. Must be overriden in inherited classes.
    The default handler is provided (doing nothing).
    @param alg :: Pointer to the algorithm sending the notification. Note that
   this can
    point to a different object than the argument of a observeZZZ(...) method,
   e.g.
    an observer can be connected to an AlgorithmProxy instance and receive
   notifications from
    the corresponding Algorithm type object.
    @param what :: The error message
*/
void AlgorithmObserver::errorHandle(const IAlgorithm *alg,
                                    const std::string &what) {
  UNUSED_ARG(alg)
  UNUSED_ARG(what)
}
/// @endcond

/** Poco notification handler for Algorithm::ProgressNotification.
    @param pNf :: An pointer to the notification.
*/
void AlgorithmObserver::_progressHandle(
    const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf) {
  this->progressHandle(pNf->algorithm(), pNf->progress, pNf->message);
}

/** Poco notification handler for Algorithm::StartedNotification.
    @param pNf :: An pointer to the notification.
*/
void AlgorithmObserver::_startHandle(
    const Poco::AutoPtr<Algorithm::StartedNotification> &pNf) {
  this->startHandle(pNf->algorithm());
}

/** Poco notification handler for Algorithm::FinishedNotification.
    @param pNf :: An pointer to the notification.
*/
void AlgorithmObserver::_finishHandle(
    const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf) {
  this->finishHandle(pNf->algorithm());
}

/** Poco notification handler for Algorithm::ErrorNotification.
    @param pNf :: An pointer to the notification.
*/
void AlgorithmObserver::_errorHandle(
    const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf) {
  this->errorHandle(pNf->algorithm(), pNf->what);
}

/** Poco notification handler for API::AlgorithmStartingNotification.
    @param pNf :: An pointer to the notification.
*/
void AlgorithmObserver::_startingHandle(
    const Poco::AutoPtr<AlgorithmStartingNotification> &pNf) {
  this->startingHandle(pNf->getAlgorithm());
}

} // namespace API
} // namespace Mantid
