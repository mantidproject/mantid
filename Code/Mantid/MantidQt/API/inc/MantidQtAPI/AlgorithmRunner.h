#ifndef MANTID_API_ALGORITHMRUNNER_H_
#define MANTID_API_ALGORITHMRUNNER_H_

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"

#include <QObject>
#include <Poco/NObserver.h>

namespace MantidQt
{
namespace API
{
  /** The AlgorithmRunner is a QObject that encapsulates
   * methods for running an algorithm asynchronously (in the background)
   * and feeds-back to a GUI widget.
   *
   * The QObject keeps track of a running algorithm.
   * Any already-running algorithm is cancelled if it gets started again.
   * Signals are emitted when the algorithm progresses or finishes.
   *
   * TO USE:
   *  - Create the AlgorithmRunner object.
   *  - Connect the desired signal(s) to slots on your GUI.
   *  - Call startAlgorithm() to start.
    
    @date 2012-04-23

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
  class EXPORT_OPT_MANTIDQT_API AlgorithmRunner : public QObject
  {
    Q_OBJECT

  public:
    explicit AlgorithmRunner(QObject * parent = 0);
    virtual ~AlgorithmRunner();
    
    void cancelRunningAlgorithm();

    void startAlgorithm(Mantid::API::IAlgorithm_sptr alg);
    Mantid::API::IAlgorithm_sptr getAlgorithm() const;

  signals:
    /// Signal emitted when the algorithm has completed execution/encountered an error
    void algorithmComplete(bool error);

    /// Signal emitted when the algorithm reports progress
    void algorithmProgress(double p, const std::string& msg);

  protected:

    /// Algorithm notification handlers
    void handleAlgorithmFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf);
    Poco::NObserver<AlgorithmRunner, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;

    void handleAlgorithmProgressNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf);
    Poco::NObserver<AlgorithmRunner, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;

    void handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf);
    Poco::NObserver<AlgorithmRunner, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;

    /// For the asynchronous call in dynamic rebinning. Holds the result of asyncExecute() algorithm call
    Poco::ActiveResult<bool> * m_asyncResult;

    /// Reference to the algorithm executing asynchronously.
    Mantid::API::IAlgorithm_sptr m_asyncAlg;

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_ALGORITHMRUNNER_H_ */
