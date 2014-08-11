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
    m_algRunner(new AlgorithmRunner(this)), m_stopOnFailure(true), m_isExecuting(false)
  {
    connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(subAlgorithmFinished(bool)));
    connect(m_algRunner, SIGNAL(algorithmProgress(double, const std::string &)), this, SLOT(subAlgorithmProgress(double, const std::string &)));
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
   */
  void BatchAlgorithmRunner::addAlgorithm(IAlgorithm_sptr algo)
  {
    if(m_isExecuting)
    {
      g_log.warning("Cannot add algorithm to queue whilst it is executing");
      return;
    }

    m_algorithms.push_back(algo);

    g_log.debug() << "Added algorithm \"" << m_algorithms.back()->name() << "\" to batch queue\n";
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
      m_algRunner->cancelRunningAlgorithm();
    }

    m_isExecuting = false;
  }

  /**
   * Creates empty workspaces in the ADS prior to queue execution
   *
   * This is important as some algorithms may depend on the output workspaces of
   * algorithms earlier in the queue, which until they are  executed will not be in
   * the ADS hence setting the property of an input or inout workspace will give an error.
   *
   * By creating an empty workspace with the same name, WorkspaceProperty is happy as it
   * is able to find a workspace with the given name, which is replaces with the intended
   * data when an algorithm earlier in the queue executes.
   *
   * @param workspaceName Name of empty workspace tpo create
   */
  void BatchAlgorithmRunner::preRegisterWorkspace(std::string workspaceName)
  {
    IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("CreateWorkspace");
    createWsAlg->initialize();

    createWsAlg->setProperty("DataX", "0");
    createWsAlg->setProperty("DataY", "0");
    createWsAlg->setProperty("OutputWorkspace", workspaceName);

    // Should be OK to run this on same thread, it is doing very little work
    createWsAlg->execute();

    g_log.debug() << "Created empty workspace \"" << workspaceName << "\" in ADS\n";
  }

  /**
   * Creates multiple empty workspaces in ADS
   *
   * @see preRegisterWorkspace()
   *
   * @param workspaceNames Vector of workspace names to create
   */
  void BatchAlgorithmRunner::preRegisterWorkspaces(std::vector<std::string> workspaceNames)
  {
    for(auto it = workspaceNames.begin(); it != workspaceNames.end(); ++it)
    {
      preRegisterWorkspace(*it);
    }
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
   * Slot fired when an algorithm finished execution
   *
   * Starts execution of following algorithm
   *
   * @param error If the algorithm failed
   */
  void BatchAlgorithmRunner::subAlgorithmFinished(bool error)
  {
    if(error)
    {
      g_log.warning() << "Got error from algorithm \"" << m_algRunner->getAlgorithm()->name() << "\"\n";

      if(m_stopOnFailure)
      {
        g_log.warning("Stopping batch algorithm because of execution error");
        cancelAll();
        return;
      }
    }

    startNextAlgo();
  }

  /**
   * Slot fired when an algorithm reports it's progress
   *
   * Used to emmit the batchProgress signal
   *
   * @param p Percentage completion
   * @param msg Progress message
   */
  void BatchAlgorithmRunner::subAlgorithmProgress(double p, const std::string& msg)
  {
    double percentPerAlgo = (1.0 / (double)m_batchSize);
    double batchPercentDone = (percentPerAlgo * static_cast<double>(m_batchSize - m_algorithms.size() - 1))
      + (percentPerAlgo * p);

    std::string currentAlgo = m_algRunner->getAlgorithm()->name();

    emit batchProgress(batchPercentDone, currentAlgo, msg);
  }

  /**
   * Starts the next anglorithm in the queue
   */
  void BatchAlgorithmRunner::startNextAlgo()
  {
    if(!m_algorithms.empty())
    {
      IAlgorithm_sptr nextAlgo = m_algorithms.front();
      m_algorithms.pop_front();

      g_log.information() << "Starting next algorithm in queue: " << nextAlgo->name() << "\n";

      m_algRunner->startAlgorithm(nextAlgo);

      m_isExecuting = true;
    }
    else
    {
      g_log.information("Batch algorithm queue empty");
      m_isExecuting = false;
    }
  }

} // namespace Mantid
} // namespace API
