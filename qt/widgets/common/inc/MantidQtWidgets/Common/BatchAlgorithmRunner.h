// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IConfiguredAlgorithm.h"
#include "MantidAPI/Algorithm.h"

#include <QObject>

#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/NObserver.h>
#include <Poco/Void.h>

#include <QMetaType>
#include <deque>
#include <mutex>
#include <utility>

namespace MantidQt::API {

class BatchCompleteNotification : public Poco::Notification {
public:
  BatchCompleteNotification(bool inProgress, bool error)
      : Poco::Notification(), m_inProgress(inProgress), m_error(error) {}

  bool isInProgress() const { return m_inProgress; }
  bool hasError() const { return m_error; }

private:
  bool m_inProgress;
  bool m_error;
};

class BatchCancelledNotification : public Poco::Notification {
public:
  BatchCancelledNotification() : Poco::Notification() {}
};

class AlgorithmCompleteNotification : public Poco::Notification {
public:
  AlgorithmCompleteNotification(IConfiguredAlgorithm_sptr algorithm)
      : Poco::Notification(), m_algorithm(std::move(algorithm)) {}

  IConfiguredAlgorithm_sptr algorithm() const { return m_algorithm; }

private:
  IConfiguredAlgorithm_sptr m_algorithm;
};

class AlgorithmStartedNotification : public Poco::Notification {
public:
  AlgorithmStartedNotification(IConfiguredAlgorithm_sptr algorithm)
      : Poco::Notification(), m_algorithm(std::move(algorithm)) {}

  IConfiguredAlgorithm_sptr algorithm() const { return m_algorithm; }

private:
  IConfiguredAlgorithm_sptr m_algorithm;
};

class AlgorithmErrorNotification : public Poco::Notification {
public:
  AlgorithmErrorNotification(IConfiguredAlgorithm_sptr algorithm, std::string const &errorMessage)
      : Poco::Notification(), m_algorithm(std::move(algorithm)), m_errorMessage(errorMessage) {}

  IConfiguredAlgorithm_sptr algorithm() const { return m_algorithm; }
  std::string errorMessage() const { return m_errorMessage; }

private:
  IConfiguredAlgorithm_sptr m_algorithm;
  std::string m_errorMessage;
};

/**
 * Algorithm runner for execution of a queue of algorithms

 @date 2014-08-10
*/

class EXPORT_OPT_MANTIDQT_COMMON BatchAlgorithmRunner : public QObject {
  Q_OBJECT

public:
  explicit BatchAlgorithmRunner(QObject *parent = nullptr);
  ~BatchAlgorithmRunner() override;

  /// Adds an algorithm to the execution queue
  void addAlgorithm(const Mantid::API::IAlgorithm_sptr &algo);
  void addAlgorithm(const Mantid::API::IAlgorithm_sptr &algo,
                    std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> props);

  void setQueue(std::deque<IConfiguredAlgorithm_sptr> algorithm);
  /// Clears all algorithms from queue
  void clearQueue();
  /// Gets size of queue
  size_t queueLength();

  /// Executes the batch, waits for the result and returns it
  bool executeBatch();
  /// Starts the batch executing and returns immediately
  void executeBatchAsync();
  /// Starts a single algorithm and returns immediately
  void executeAlgorithmAsync(IConfiguredAlgorithm_sptr algorithm);
  /// Request to cancel processing the batch
  void cancelBatch();

  /// Sets if the execuion should be stopped if an error is detected
  void stopOnFailure(bool stopOnFailure);

signals:
  /// Emitted when a batch has finished executing
  void batchComplete(bool error);
  void batchCancelled();
  void algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm);
  void algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm);
  void algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm, std::string errorMessage);

private:
  /// Implementation of algorithm runner
  bool executeBatchAsyncImpl(const Poco::Void & /*unused*/);
  /// Sets up and executes an algorithm
  bool executeAlgo(const IConfiguredAlgorithm_sptr &algorithm);
  /// Post a poco notification
  void postNotification(Poco::Notification *notification);
  // Sets cancel requested flag
  void setCancelRequested(bool const cancel);

  /// Handlers for notifications
  void handleBatchComplete(const Poco::AutoPtr<BatchCompleteNotification> &pNf);
  void handleBatchCancelled(const Poco::AutoPtr<BatchCancelledNotification> &pNf);
  void handleAlgorithmStarted(const Poco::AutoPtr<AlgorithmStartedNotification> &pNf);
  void handleAlgorithmComplete(const Poco::AutoPtr<AlgorithmCompleteNotification> &pNf);
  void handleAlgorithmError(const Poco::AutoPtr<AlgorithmErrorNotification> &pNf);

  /// The queue of algorithms to be executed
  std::deque<IConfiguredAlgorithm_sptr> m_algorithms;

  /// The current algorithm being executed
  Mantid::API::IAlgorithm_sptr m_currentAlgorithm;

  /// If execution should be stopped on algorithm failure
  bool m_stopOnFailure;

  /// User has requested to cancel processing
  bool m_cancelRequested;
  std::recursive_mutex m_executeMutex;
  std::recursive_mutex m_cancelMutex;
  std::recursive_mutex m_notificationMutex;
  void resetState();
  bool cancelRequested();

  /// Notification center used to handle notifications from active method
  mutable Poco::NotificationCenter m_notificationCenter;
  /// Observer for notifications
  Poco::NObserver<BatchAlgorithmRunner, BatchCompleteNotification> m_batchCompleteObserver;
  Poco::NObserver<BatchAlgorithmRunner, BatchCancelledNotification> m_batchCancelledObserver;
  Poco::NObserver<BatchAlgorithmRunner, AlgorithmStartedNotification> m_algorithmStartedObserver;
  Poco::NObserver<BatchAlgorithmRunner, AlgorithmCompleteNotification> m_algorithmCompleteObserver;
  Poco::NObserver<BatchAlgorithmRunner, AlgorithmErrorNotification> m_algorithmErrorObserver;

  /// Active method to run batch runner on separate thread
  Poco::ActiveMethod<bool, Poco::Void, BatchAlgorithmRunner, Poco::ActiveStarter<BatchAlgorithmRunner>> m_executeAsync;
  /// Holds result of async execution
  Poco::ActiveResult<bool> executeAsync();

  void addAllObservers();
  void removeAllObservers();
};
} // namespace MantidQt::API
