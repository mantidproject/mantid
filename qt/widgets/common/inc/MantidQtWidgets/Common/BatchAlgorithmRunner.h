// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_BATCHALGORITHMRUNNER_H_
#define MANTID_API_BATCHALGORITHMRUNNER_H_

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"

#include <QObject>

#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/NObserver.h>
#include <Poco/Void.h>

#include <deque>

namespace MantidQt {
namespace API {

class BatchAlgorithmRunnerSubscriber {
public:
  virtual void notifyAlgorithmStarted() = 0;
  virtual void notifyAlgorithmComplete() = 0;
  virtual void notifyAlgorithmError(std::string const &msg) = 0;
};

class ConfiguredAlgorithm {
public:
  using AlgorithmRuntimeProps = std::map<std::string, std::string>;

  ConfiguredAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                      AlgorithmRuntimeProps properties,
                      BatchAlgorithmRunnerSubscriber *notifyee = nullptr)
      : m_algorithm(algorithm), m_properties(std::move(properties)),
        m_notifyee(notifyee) {}

  Mantid::API::IAlgorithm_sptr algorithm() const { return m_algorithm; }
  AlgorithmRuntimeProps properties() const { return m_properties; }
  BatchAlgorithmRunnerSubscriber *notifyee() const { return m_notifyee; }

private:
  Mantid::API::IAlgorithm_sptr m_algorithm;
  AlgorithmRuntimeProps m_properties;
  BatchAlgorithmRunnerSubscriber *m_notifyee;
};

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

class AlgorithmCompleteNotification : public Poco::Notification {
public:
  AlgorithmCompleteNotification(BatchAlgorithmRunnerSubscriber *notifyee)
      : Poco::Notification(), m_notifyee(notifyee) {}

  BatchAlgorithmRunnerSubscriber *notifyee() const { return m_notifyee; }

private:
  BatchAlgorithmRunnerSubscriber *m_notifyee;
};

class AlgorithmErrorNotification : public Poco::Notification {
public:
  AlgorithmErrorNotification(BatchAlgorithmRunnerSubscriber *notifyee,
                             std::string const &errorMessage)
      : Poco::Notification(), m_notifyee(notifyee),
        m_errorMessage(errorMessage) {}

  BatchAlgorithmRunnerSubscriber *notifyee() const { return m_notifyee; }
  std::string errorMessage() const { return m_errorMessage; }

private:
  BatchAlgorithmRunnerSubscriber *m_notifyee;
  std::string m_errorMessage;
};

/**
 * Algorithm runner for execution of a queue of algorithms

 @date 2014-08-10
*/

class EXPORT_OPT_MANTIDQT_COMMON BatchAlgorithmRunner : public QObject {
  Q_OBJECT

public:
  using AlgorithmRuntimeProps = std::map<std::string, std::string>;

  explicit BatchAlgorithmRunner(QObject *parent = nullptr);
  ~BatchAlgorithmRunner() override;

  /// Adds an algorithm to the execution queue
  void addAlgorithm(Mantid::API::IAlgorithm_sptr algo,
                    AlgorithmRuntimeProps props = AlgorithmRuntimeProps(),
                    BatchAlgorithmRunnerSubscriber *notifyee = nullptr);
  /// Clears all algorithms from queue
  void clearQueue();
  /// Gets size of queue
  size_t queueLength();

  /// Executes the batch, waits for the result and returns it
  bool executeBatch();
  /// Starts the batch executing and returns immediately
  void executeBatchAsync();

  /// Sets if the execuion should be stopped if an error is detected
  void stopOnFailure(bool stopOnFailure);

signals:
  /// Emitted when a batch has finished executing
  void batchComplete(bool error);
  void algorithmComplete();
  void algorithmError(std::string const &errorMessage);

private:
  /// Implementation of algorithm runner
  bool executeBatchAsyncImpl(const Poco::Void &);
  /// Sets up and executes an algorithm
  bool executeAlgo(ConfiguredAlgorithm &algorithm);

  /// Handler for batch completion
  void handleBatchComplete(const Poco::AutoPtr<BatchCompleteNotification> &pNf);
  void handleAlgorithmComplete(
      const Poco::AutoPtr<AlgorithmCompleteNotification> &pNf);
  void
  handleAlgorithmError(const Poco::AutoPtr<AlgorithmErrorNotification> &pNf);

  /// The queue of algorithms to be executed
  std::deque<ConfiguredAlgorithm> m_algorithms;

  /// The current algorithm being executed
  Mantid::API::IAlgorithm_sptr m_currentAlgorithm;

  /// If execution should be stopped on algorithm failure
  bool m_stopOnFailure;

  /// Notification center used to handle notifications from active method
  mutable Poco::NotificationCenter m_notificationCenter;
  /// Observer for notifications
  Poco::NObserver<BatchAlgorithmRunner, BatchCompleteNotification>
      m_batchCompleteObserver;
  Poco::NObserver<BatchAlgorithmRunner, AlgorithmCompleteNotification>
      m_algorithmCompleteObserver;
  Poco::NObserver<BatchAlgorithmRunner, AlgorithmErrorNotification>
      m_algorithmErrorObserver;

  /// Active method to run batch runner on separate thread
  Poco::ActiveMethod<bool, Poco::Void, BatchAlgorithmRunner,
                     Poco::ActiveStarter<BatchAlgorithmRunner>>
      m_executeAsync;
  /// Holds result of async execution
  Poco::ActiveResult<bool> executeAsync();

  void addAllObservers();
  void removeAllObservers();
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_BATCHALGORITHMRUNNER_H_ */
