#ifndef MANTID_API_ABSTRACTASYNCALGORITHMRUNNER_H_
#define MANTID_API_ABSTRACTASYNCALGORITHMRUNNER_H_

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"

#include <QObject>
#include <Poco/NObserver.h>

namespace MantidQt
{
namespace API
{

  /**TODO
    
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
  class EXPORT_OPT_MANTIDQT_API AbstractAsyncAlgorithmRunner : public QObject
  {
    Q_OBJECT

  public:
    explicit AbstractAsyncAlgorithmRunner(QObject * parent = 0);
    virtual ~AbstractAsyncAlgorithmRunner();
    
    Mantid::API::IAlgorithm_sptr getCurrentAlgorithm() const;

  protected:
    void startAlgorithm(Mantid::API::IAlgorithm_sptr alg);
    void cancelRunningAlgorithm();

    /// Algorithm notification handlers
    void handleAlgorithmFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf);
    Poco::NObserver<AbstractAsyncAlgorithmRunner, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;
    virtual void handleAlgorithmFinish() = 0;
    
    void handleAlgorithmProgressNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf);
    Poco::NObserver<AbstractAsyncAlgorithmRunner, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;
    virtual void handleAlgorithmProgress(double p, std::string msg) = 0;

    void handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf);
    Poco::NObserver<AbstractAsyncAlgorithmRunner, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;
    virtual void handleAlgorithmError() = 0;

    /// For the asynchronous call in dynamic rebinning. Holds the result of asyncExecute() algorithm call
    Poco::ActiveResult<bool> * m_asyncResult;

    /// Reference to the algorithm executing asynchronously.
    Mantid::API::IAlgorithm_sptr m_asyncAlg;
  };

} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_ABSTRACTASYNCALGORITHMRUNNER_H_ */
