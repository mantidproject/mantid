// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("BatchAlgorithmRunner");
}

namespace MantidQt {
namespace API {

ConfiguredAlgorithm::ConfiguredAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                                         AlgorithmRuntimeProps properties)
    : m_algorithm(algorithm), m_properties(std::move(properties)) {}

ConfiguredAlgorithm::~ConfiguredAlgorithm() {}

IAlgorithm_sptr ConfiguredAlgorithm::algorithm() const { return m_algorithm; }

ConfiguredAlgorithm::AlgorithmRuntimeProps
ConfiguredAlgorithm::properties() const {
  return m_properties;
}

BatchAlgorithmRunner::BatchAlgorithmRunner(QObject *parent)
    : QObject(parent), m_stopOnFailure(true), m_cancelRequested(false),
      m_notificationCenter(),
      m_batchCompleteObserver(*this,
                              &BatchAlgorithmRunner::handleBatchComplete),
      m_batchCancelledObserver(*this,
                               &BatchAlgorithmRunner::handleBatchCancelled),
      m_algorithmStartedObserver(*this,
                                 &BatchAlgorithmRunner::handleAlgorithmStarted),
      m_algorithmCompleteObserver(
          *this, &BatchAlgorithmRunner::handleAlgorithmComplete),
      m_algorithmErrorObserver(*this,
                               &BatchAlgorithmRunner::handleAlgorithmError),
      m_executeAsync(this, &BatchAlgorithmRunner::executeBatchAsyncImpl) {}

BatchAlgorithmRunner::~BatchAlgorithmRunner() { removeAllObservers(); }

void BatchAlgorithmRunner::addAllObservers() {
  removeAllObservers();
  m_notificationCenter.addObserver(m_batchCompleteObserver);
  m_notificationCenter.addObserver(m_batchCancelledObserver);
  m_notificationCenter.addObserver(m_algorithmStartedObserver);
  m_notificationCenter.addObserver(m_algorithmCompleteObserver);
  m_notificationCenter.addObserver(m_algorithmErrorObserver);
}

void BatchAlgorithmRunner::removeAllObservers() {
  m_notificationCenter.removeObserver(m_batchCompleteObserver);
  m_notificationCenter.removeObserver(m_batchCancelledObserver);
  m_notificationCenter.removeObserver(m_algorithmStartedObserver);
  m_notificationCenter.removeObserver(m_algorithmCompleteObserver);
  m_notificationCenter.removeObserver(m_algorithmErrorObserver);
}

/**
 * Sets if the execution of the queue should be stopped if an algorithm fails.
 *
 * Defaults to true
 *
 * @param stopOnFailure Flase to continue to tnd of queue on failure
 */
void BatchAlgorithmRunner::stopOnFailure(bool stopOnFailure) {
  m_stopOnFailure = stopOnFailure;
}

/**
 * Adds an algorithm to the end of the queue.
 *
 * @param algo Algorithm to add to queue
 * @param props Optional map of property name to property values to be set just
 *before execution (mainly intended for input and inout workspace names)
 */
void BatchAlgorithmRunner::addAlgorithm(IAlgorithm_sptr algo,
                                        AlgorithmRuntimeProps props) {
  m_algorithms.emplace_back(
      Mantid::Kernel::make_unique<ConfiguredAlgorithm>(algo, props));

  g_log.debug() << "Added algorithm \""
                << m_algorithms.back()->algorithm()->name()
                << "\" to batch queue\n";
}

/**
 * Set the queue of algorithms
 *
 * @param algorithms The queue of configured algorithms
 */
void BatchAlgorithmRunner::setQueue(
    std::deque<IConfiguredAlgorithm_sptr> algorithms) {
  g_log.debug() << "Set batch queue to algorithm list:\n";
  for (auto &algorithm : algorithms)
    g_log.debug() << algorithm->algorithm()->name() << "\n";

  m_algorithms = std::move(algorithms);
}

/**
 * Removes all algorithms from the queue.
 */
void BatchAlgorithmRunner::clearQueue() { m_algorithms.clear(); }

/**
 * Returns the number of algorithms in the queue.
 */
size_t BatchAlgorithmRunner::queueLength() { return m_algorithms.size(); }

/**
 * Executes the algorithms on a separate thread and waits for their completion.
 *
 * @return False if the batch was stopped due to error
 */
bool BatchAlgorithmRunner::executeBatch() {
  addAllObservers();
  Poco::ActiveResult<bool> result = m_executeAsync(Poco::Void());
  result.wait();
  removeAllObservers();
  return result.data();
}

/**
 * Starts executing the queue of algorithms on a separate thread.
 */
void BatchAlgorithmRunner::executeBatchAsync() {
  addAllObservers();
  Poco::ActiveResult<bool> result = m_executeAsync(Poco::Void());
}

/**
 * Cancel execution of remaining queued items
 */
void BatchAlgorithmRunner::cancelBatch() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_cancelRequested = true;
}

/**
 * Reset state ready for executing a new batch
 */
void BatchAlgorithmRunner::resetState() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_cancelRequested = false;
}

bool BatchAlgorithmRunner::cancelRequested() {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_cancelRequested;
}

/**
 * Implementation of sequential algorithm scheduler.
 */
bool BatchAlgorithmRunner::executeBatchAsyncImpl(
    const Poco::Void & /*unused*/) {
  bool errorFlag = false;

  for (auto &it : m_algorithms) {
    if (cancelRequested()) {
      g_log.information("Stopping batch algorithm execution: cancelled");
      break;
    }

    // Try to execute the algorithm
    if (!executeAlgo(it)) {
      g_log.warning() << "Got error from algorithm \""
                      << m_currentAlgorithm->name() << "\"\n";

      // Stop executing the entire batch if appropriate
      if (m_stopOnFailure) {
        g_log.warning("Stopping batch algorithm because of execution error");
        errorFlag = true;
        break;
      }
    } else {
      g_log.information() << "Algorithm \"" << m_currentAlgorithm->name()
                          << "\" finished\n";
    }
  }

  // Clear queue
  m_algorithms.clear();

  m_notificationCenter.postNotification(
      new BatchCompleteNotification(false, errorFlag));
  removeAllObservers();

  return !errorFlag;
}

/**
 * Assigns properties to an algorithm then executes it
 *
 * @param algorithm Algorithm and properties to assign to it
 * @return False if algorithm execution failed
 */
bool BatchAlgorithmRunner::executeAlgo(IConfiguredAlgorithm_sptr algorithm) {
  try {
    m_currentAlgorithm = algorithm->algorithm();

    // Assign the properties to be set at runtime
    for (auto const &kvp : algorithm->properties()) {
      m_currentAlgorithm->setProperty(kvp.first, kvp.second);
    }

    g_log.information() << "Starting next algorithm in queue: "
                        << m_currentAlgorithm->name() << "\n";

    // Start algorithm running
    m_notificationCenter.postNotification(
        new AlgorithmStartedNotification(algorithm));
    auto result = m_currentAlgorithm->execute();

    if (!result) {
      auto message = std::string("Algorithm") + algorithm->algorithm()->name() +
                     std::string(" execution failed");
      m_notificationCenter.postNotification(
          new AlgorithmErrorNotification(algorithm, message));
    } else {
      m_notificationCenter.postNotification(
          new AlgorithmCompleteNotification(algorithm));
    }

    return result;
  }
  // If a property name was given that does not match a property
  catch (Mantid::Kernel::Exception::NotFoundError &notFoundEx) {
    UNUSED_ARG(notFoundEx);
    g_log.warning(
        "Algorithm property does not exist.\nStopping queue execution.");
    m_notificationCenter.postNotification(
        new AlgorithmErrorNotification(algorithm, notFoundEx.what()));
    return false;
  }
  // If a property was assigned a value of the wrong type
  catch (std::invalid_argument &invalidArgEx) {
    UNUSED_ARG(invalidArgEx);
    g_log.warning("Algorithm property given value of incorrect type.\nStopping "
                  "queue execution.");
    m_notificationCenter.postNotification(
        new AlgorithmErrorNotification(algorithm, invalidArgEx.what()));
    return false;
  }
  // For anything else that could go wrong
  catch (std::exception &ex) {
    g_log.warning("Error starting batch algorithm");
    m_notificationCenter.postNotification(
        new AlgorithmErrorNotification(algorithm, ex.what()));
    return false;
  } catch (...) {
    g_log.warning("Unknown error starting next batch algorithm");
    m_notificationCenter.postNotification(new AlgorithmErrorNotification(
        algorithm, "Unknown error starting algorithm"));
    return false;
  }
}

/**
 * Handles the notification posted when the algorithm queue stops execution.
 *
 * @param pNf Notification object
 */
void BatchAlgorithmRunner::handleBatchComplete(
    const Poco::AutoPtr<BatchCompleteNotification> &pNf) {
  bool inProgress = pNf->isInProgress();
  if (!inProgress) {
    // Notify UI elements
    emit batchComplete(pNf->hasError());
  }
}

void BatchAlgorithmRunner::handleBatchCancelled(
    const Poco::AutoPtr<BatchCancelledNotification> &pNf) {
  UNUSED_ARG(pNf);
  // Notify UI elements
  emit batchCancelled();
}

void BatchAlgorithmRunner::handleAlgorithmStarted(
    const Poco::AutoPtr<AlgorithmStartedNotification> &pNf) {
  // Notify UI elements
  emit algorithmStarted(pNf->algorithm());
}

void BatchAlgorithmRunner::handleAlgorithmComplete(
    const Poco::AutoPtr<AlgorithmCompleteNotification> &pNf) {
  // Notify UI elements
  emit algorithmComplete(pNf->algorithm());
}

void BatchAlgorithmRunner::handleAlgorithmError(
    const Poco::AutoPtr<AlgorithmErrorNotification> &pNf) {
  auto errorMessage = pNf->errorMessage();
  // Notify UI elements
  emit algorithmError(pNf->algorithm(), errorMessage);
}
} // namespace API
} // namespace MantidQt
