#include "MantidQtAPI/BatchAlgorithmRunner.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("BatchAlgorithmRunner");
}

namespace MantidQt
{
namespace API
{
  BatchAlgorithmRunner::BatchAlgorithmRunner(QObject * parent) : QObject(parent),
    m_stopOnFailure(true),
    m_executeAsync(this, &BatchAlgorithmRunner::executeBatchAsyncImpl),
    m_notificationObserver(*this, &BatchAlgorithmRunner::handleNotification)
  {
  }
    
  BatchAlgorithmRunner::~BatchAlgorithmRunner()
  {
  }

  /**
   * Gets a pointer to the current algorithm being executed
   *
   * @return Current algorithm
   */
  IAlgorithm_sptr BatchAlgorithmRunner::getCurrentAlgorithm()
  {
    return m_currentAlgorithm;
  }

  void BatchAlgorithmRunner::stopOnFailure(bool stopOnFailure)
  {
    m_stopOnFailure = stopOnFailure;
  }

  /**
   * Adds an algorithm to the end of the queue
   *
   * @param algo Algorithm to add to queue
   * @param props Optional map of property name to property values to be set just before execution (mainly intended for input and inout workspace names)
   */
  void BatchAlgorithmRunner::addAlgorithm(IAlgorithm_sptr algo, AlgorithmRuntimeProps props)
  {
    m_algorithms.push_back(std::make_pair(algo, props));

    g_log.debug() << "Added algorithm \"" << m_algorithms.back().first->name() << "\" to batch queue\n";
  }

  /**
   * Executes the algorithms on a separate thread and waits for their completion.
   *
   * @return False if the batch was stopped due to error
   */
  bool BatchAlgorithmRunner::executeBatch()
  {
    notificationCenter().addObserver(m_notificationObserver);
    Poco::ActiveResult<bool> result = m_executeAsync(Poco::Void());
    result.wait();
    notificationCenter().removeObserver(m_notificationObserver);
    return result.data();
  }
  
  /**
   * Starts executing the queue of algorithms on a separate thread.
   */
  void BatchAlgorithmRunner::executeBatchAsync()
  {
    notificationCenter().addObserver(m_notificationObserver);
    Poco::ActiveResult<bool> result = m_executeAsync(Poco::Void());
  }

  /**
   * Implementation of sequential algorithm scheduler
   */
  bool BatchAlgorithmRunner::executeBatchAsyncImpl(const Poco::Void&)
  {
    bool cancelFlag = false;

    for(auto it = m_algorithms.begin(); it != m_algorithms.end(); ++it)
    {
      if(!startAlgo(*it))
      {    
        g_log.warning() << "Got error from algorithm \"" << getCurrentAlgorithm()->name() << "\"\n";
        if(m_stopOnFailure)
        {
          g_log.warning("Stopping batch algorithm because of execution error");
          notificationCenter().postNotification(new BatchNotification(false, true));
          cancelFlag = true;
          break;
        }
      }
      else
      {
        g_log.information() << "Algorithm \"" << getCurrentAlgorithm()->name() << "\" finished\n";
      }
    }

    // Clear queue
    m_algorithms.clear();

    if(cancelFlag)
    {
      return false;
    }

    notificationCenter().postNotification(new BatchNotification(false, false));
    notificationCenter().removeObserver(m_notificationObserver);

    return true;
  }

  /**
   * Assigns properties to an algorithm then starts it
   *
   * @param algorithm Algorithm and properties to assign to it
   * @return False if algorithm execution failed
   */
  bool BatchAlgorithmRunner::startAlgo(ConfiguredAlgorithm algorithm)
  {
    try
    {
      m_currentAlgorithm = algorithm.first;

      // Assign the properties to be set at runtime
      for(auto it = algorithm.second.begin(); it != algorithm.second.end(); ++it)
      {
        m_currentAlgorithm->setProperty(it->first, it->second);
      }

      g_log.information() << "Starting next algorithm in queue: " << m_currentAlgorithm->name() << "\n";

      // Start algorithm running
      return m_currentAlgorithm->execute();
    }
    // If a property name was given that does not match a property
    catch(Mantid::Kernel::Exception::NotFoundError &notFoundEx)
    {
      UNUSED_ARG(notFoundEx);
      g_log.warning("Algorithm property does not exist.\nStopping queue execution.");
      return false;
    }
    // If a property was assigned a value of the wrong type
    catch(std::invalid_argument &invalidArgEx)
    {
      UNUSED_ARG(invalidArgEx);
      g_log.warning("Algorithm property given value of incorrect type.\nStopping queue execution.");
      return false;
    }
    catch(...)
    {
      g_log.warning("Unknown error starting next batch algorithm");
      return false;
    }
  }

  /**
   * Gets a reference to the notification center used to process Poco notifications.
   *
   * @return Notification Center
   */
  Poco::NotificationCenter & BatchAlgorithmRunner::notificationCenter() const
  {
    if(!m_notificationCenter) m_notificationCenter = new Poco::NotificationCenter;
    return *m_notificationCenter;
  }

  /**
   * Handles the notification posted when the algorithm queue stops execution.
   *
   * @param pNf Notification object
   */
  void BatchAlgorithmRunner::handleNotification(const Poco::AutoPtr<BatchNotification>& pNf)
  {
    bool inProgress = pNf->isInProgress();
    if(!inProgress)
    {
      emit batchComplete(pNf->hasError());
    }
  }

} // namespace Mantid
} // namespace API
