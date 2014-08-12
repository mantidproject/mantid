#include "MantidQtAPI/AbstractAsyncAlgorithmRunner.h"

#include <Poco/ActiveResult.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AbstractAsyncAlgorithmRunner::AbstractAsyncAlgorithmRunner(QObject * parent) : QObject(parent),
    m_finishedObserver(*this, &AbstractAsyncAlgorithmRunner::handleAlgorithmFinishedNotification),
    m_progressObserver(*this, &AbstractAsyncAlgorithmRunner::handleAlgorithmProgressNotification),
    m_errorObserver(*this, &AbstractAsyncAlgorithmRunner::handleAlgorithmErrorNotification),
    m_asyncResult(NULL)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AbstractAsyncAlgorithmRunner::~AbstractAsyncAlgorithmRunner()
  {
    if (m_asyncAlg)
    {
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
  void AbstractAsyncAlgorithmRunner::cancelRunningAlgorithm()
  {
    // Cancel any currently running algorithms
    if (m_asyncAlg)
    {
      if (m_asyncAlg->isRunning())
      {
        m_asyncAlg->cancel();
      }
      if (m_asyncResult)
      {
        m_asyncResult->tryWait(1000);
        delete m_asyncResult;
        m_asyncResult = NULL;
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
   * @param alg :: algorithm to execute. All properties should have been set properly.
   */
  void AbstractAsyncAlgorithmRunner::startAlgorithm(Mantid::API::IAlgorithm_sptr alg)
  {
    if (!alg)
      throw std::invalid_argument("AbstractAsyncAlgorithmRunner::startAlgorithm() given a NULL Algorithm");
    if (!alg->isInitialized())
      throw std::invalid_argument("AbstractAsyncAlgorithmRunner::startAlgorithm() given an uninitialized Algorithm");

    cancelRunningAlgorithm();

    // Start asynchronous execution
    m_asyncAlg = alg;
    m_asyncResult = new Poco::ActiveResult<bool>(m_asyncAlg->executeAsync());

    // Observe the algorithm
    alg->addObserver(m_finishedObserver);
    alg->addObserver(m_errorObserver);
    alg->addObserver(m_progressObserver);
  }

  /// Get back a pointer to the running algorithm
  Mantid::API::IAlgorithm_sptr AbstractAsyncAlgorithmRunner::getCurrentAlgorithm() const
  {
    return m_asyncAlg;
  }

  //--------------------------------------------------------------------------------------
  /** Observer called when the asynchronous algorithm has completed.
   *
   * Calls handler defined in concrete class
   *
   * @param pNf :: finished notification object.
   */
  void AbstractAsyncAlgorithmRunner::handleAlgorithmFinishedNotification(const Poco::AutoPtr<Algorithm::FinishedNotification>& pNf)
  {
    UNUSED_ARG(pNf);
    handleAlgorithmFinish();
  }

  //--------------------------------------------------------------------------------------
  /** Observer called when the async algorithm has progress to report
   *
   * Calls handler defined in concrete class
   *
   * @param pNf :: notification object
   */
  void AbstractAsyncAlgorithmRunner::handleAlgorithmProgressNotification(const Poco::AutoPtr<Algorithm::ProgressNotification>& pNf)
  {
    handleAlgorithmProgress(pNf->progress, pNf->message);
  }

  //--------------------------------------------------------------------------------------
  /** Observer called when the async algorithm has encountered an error.
   *
   * Calls handler defined in concrete class
   *
   * @param pNf :: notification object
   */
  void AbstractAsyncAlgorithmRunner::handleAlgorithmErrorNotification(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf)
  {
    UNUSED_ARG(pNf);
    handleAlgorithmError();
  }

} // namespace Mantid
} // namespace API
