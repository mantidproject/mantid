#ifndef MANTID_API_BATCHALGORITHMRUNNER_H_
#define MANTID_API_BATCHALGORITHMRUNNER_H_

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"

#include <QObject>

#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/NObserver.h>
#include <Poco/Void.h>

namespace MantidQt
{
namespace API
{
  /**
   * Algorithm runner for execution of a queue of algorithms

    @date 2014-08-10

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

  class BatchNotification : public Poco::Notification
  {
  public:
    BatchNotification(bool inProgress, bool error) : Poco::Notification(),
      m_inProgress(inProgress), m_error(error) {}

    bool isInProgress() const { return m_inProgress; }
    bool hasError() const { return m_error; }

  private:
    bool m_inProgress;
    bool m_error;
  };

  class EXPORT_OPT_MANTIDQT_API BatchAlgorithmRunner : public QObject
  {
    Q_OBJECT

  public:
    typedef std::map<std::string, std::string> AlgorithmRuntimeProps;
    typedef std::pair<Mantid::API::IAlgorithm_sptr, AlgorithmRuntimeProps> ConfiguredAlgorithm;

    explicit BatchAlgorithmRunner(QObject * parent = 0);
    virtual ~BatchAlgorithmRunner();

    /// Adds an algorithm to the execution queue
    void addAlgorithm(Mantid::API::IAlgorithm_sptr algo, AlgorithmRuntimeProps props = AlgorithmRuntimeProps());
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

  private:
    /// Implementation of algorithm runner
    bool executeBatchAsyncImpl(const Poco::Void&);
    /// Sets up and executes an algorithm
    bool executeAlgo(ConfiguredAlgorithm algorithm);

    /// Handler for batch completion
    void handleNotification(const Poco::AutoPtr<BatchNotification>& pNf);

    /// The queue of algorithms to be executed
    std::deque<ConfiguredAlgorithm> m_algorithms;

    /// The current algorithm being executed
    Mantid::API::IAlgorithm_sptr m_currentAlgorithm;

    /// If execution should be stopped on algorithm failure
    bool m_stopOnFailure;

    /// Notification center used to handle notifications from active method
    mutable Poco::NotificationCenter m_notificationCenter;
    /// Observer for notifications
    Poco::NObserver<BatchAlgorithmRunner, BatchNotification> m_notificationObserver;

    /// Active method to run batch runner on separate thread
    Poco::ActiveMethod<bool, Poco::Void, BatchAlgorithmRunner, Poco::ActiveStarter<BatchAlgorithmRunner>> m_executeAsync;
    /// Holds result of async execution
    Poco::ActiveResult<bool> executeAsync();

  };

} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_BATCHALGORITHMRUNNER_H_ */
