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

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    
    void addAlgorithm(Mantid::API::IAlgorithm_sptr algo, AlgorithmRuntimeProps props = AlgorithmRuntimeProps());

    bool executeBatch();
    void executeBatchAsync();

    void stopOnFailure(bool stopOnFailure);

  signals:
    void batchComplete(bool error);

  private:
    Mantid::API::IAlgorithm_sptr getCurrentAlgorithm();
    Poco::NotificationCenter & notificationCenter() const;

    Poco::ActiveResult<bool> executeAsync();
    bool executeBatchAsyncImpl(const Poco::Void&);

    bool startAlgo(ConfiguredAlgorithm algorithm);

    std::deque<ConfiguredAlgorithm> m_algorithms;
    size_t m_batchSize;

    Mantid::API::IAlgorithm_sptr m_currentAlgorithm;

    bool m_stopOnFailure;

    Poco::ActiveMethod<bool, Poco::Void, BatchAlgorithmRunner, Poco::ActiveStarter<BatchAlgorithmRunner>> m_executeAsync;
    mutable Poco::NotificationCenter *m_notificationCenter;

    void handleNotification(const Poco::AutoPtr<BatchNotification>& pNf);
    Poco::NObserver<BatchAlgorithmRunner, BatchNotification> m_notificationObserver;
  };

} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_BATCHALGORITHMRUNNER_H_ */
