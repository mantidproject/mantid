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
  BatchAlgorithmRunner::BatchAlgorithmRunner(QObject * parent) : AbstractAsyncAlgorithmRunner(parent),
    m_stopOnFailure(true), m_isExecuting(false)
  {
  }
    
  BatchAlgorithmRunner::~BatchAlgorithmRunner()
  {
    cancelAll();
  }
 
  /**
   * Checks to see if any algorithms are currently being executed
   *
   * @return True if algorithms are being executed, false otherwise
   */
  bool BatchAlgorithmRunner::isExecuting()
  {
    return m_isExecuting;
  }

  /**
   * Adds an algorithm to the end of the queue
   *
   * @param algo Algorithm to add to queue
   * @param props Optional map of property name to property values to be set just before execution (mainly intended for input and inout workspace names)
   */
  void BatchAlgorithmRunner::addAlgorithm(IAlgorithm_sptr algo, AlgorithmRuntimeProps props)
  {
    if(m_isExecuting)
    {
      g_log.warning("Cannot add algorithm to queue whilst it is executing");
      return;
    }

    m_algorithms.push_back(std::make_pair(algo, props));

    g_log.debug() << "Added algorithm \"" << m_algorithms.back().first->name() << "\" to batch queue\n";
  }

  /**
   * Cancels all algorithms and discards the queue
   */
  void BatchAlgorithmRunner::cancelAll()
  {
    // Clear queue
    m_algorithms.clear();

    // If an algorithm is running, stop it
    if(isExecuting())
    {
      cancelRunningAlgorithm();
    }

    m_isExecuting = false;
  }

  /**
   * Starts executing the queue of algorithms
   *
   * @param stopOnFailure Stop executing queue if an algorithm fails
   */
  void BatchAlgorithmRunner::startBatch(bool stopOnFailure)
  {
    // Do nothing if a batch is already running
    if(m_isExecuting)
    {
      return;
    }

    m_stopOnFailure = stopOnFailure;
    m_batchSize = m_algorithms.size();

    startNextAlgo();
  }

  /**
   * Handle notification when an algorithm in the queue has completed without error
   */
  void BatchAlgorithmRunner::handleAlgorithmFinish()
  {
    startNextAlgo();
  }

  /**
   * Handle notification when an algorithm reports it's progress
   *
   * This is used only to provide a Qt signal indicating the progress of the entire queue
   */
  void BatchAlgorithmRunner::handleAlgorithmProgress(const double p, const std::string msg)
  {
    double percentPerAlgo = (1.0 / (double)m_batchSize);
    double batchPercentDone = (percentPerAlgo * static_cast<double>(m_batchSize - m_algorithms.size() - 1))
      + (percentPerAlgo * p);

    std::string currentAlgo = getCurrentAlgorithm()->name();

    emit batchProgress(batchPercentDone, currentAlgo, msg);
  }

  /**
   * Handle notification when an algorithm in the queue has failed
   *
   * This can either continue to the next algorithm regardless or stop the entire queue
   */
  void BatchAlgorithmRunner::handleAlgorithmError()
  {
    g_log.warning() << "Got error from algorithm \"" << getCurrentAlgorithm()->name() << "\"\n";

    if(m_stopOnFailure)
    {
      g_log.warning("Stopping batch algorithm because of execution error");
      cancelAll();
      return;
    }

    startNextAlgo();
  }

  /**
   * Starts the next anglorithm in the queue
   */
  void BatchAlgorithmRunner::startNextAlgo()
  {
    if(!m_algorithms.empty())
    {
      ConfiguredAlgorithm nextAlgo = m_algorithms.front();
      m_algorithms.pop_front();

      try
      {
        // Assign the properties to be set ar runtime
        for(auto it = nextAlgo.second.begin(); it != nextAlgo.second.end(); ++it)
        {
          nextAlgo.first->setProperty(it->first, it->second);
        }

        // Start algorithm running
        g_log.information() << "Starting next algorithm in queue: " << nextAlgo.first->name() << "\n";
        startAlgorithm(nextAlgo.first);

        // Set execution flag
        m_isExecuting = true;
      }
      // If a property name was given that does not match a property
      catch(Mantid::Kernel::Exception::NotFoundError notFoundEx)
      {
        UNUSED_ARG(notFoundEx);

        g_log.warning("Algorithm property does not exist.\nStopping queue execution.");

        m_isExecuting = false;
        emit batchComplete(true);
      }
      // If a property was assigned a value of the wrong type
      catch(std::invalid_argument invalidArgEx)
      {
        UNUSED_ARG(invalidArgEx);

        g_log.warning("Algorithm property given value of incorrect type.\nStopping queue execution.");

        m_isExecuting = false;
        emit batchComplete(true);
      }
    }
    else
    {
      // Reached end of queue, notify GUI
      g_log.information("Batch algorithm queue empty");
      m_isExecuting = false;
      emit batchComplete(false);
    }
  }

} // namespace Mantid
} // namespace API
