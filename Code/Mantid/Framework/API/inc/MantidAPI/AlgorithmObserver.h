#ifndef MANTID_API_ALGORITHMOBSERVER_H_
#define MANTID_API_ALGORITHMOBSERVER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include <Poco/NObserver.h>

namespace Mantid
{
namespace API
{
/**
 Observes Algorithm notifications: start,progress,finish,error.
 Hides Poco::Notification API from the user. 

 Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
 */
class MANTID_API_DLL AlgorithmObserver
{
public:
    AlgorithmObserver();
    AlgorithmObserver(IAlgorithm_const_sptr alg);
    virtual ~AlgorithmObserver();

    void observeAll(IAlgorithm_const_sptr alg);
    void observeProgress(IAlgorithm_const_sptr alg);
    void observeStarting();
    void observeStart(IAlgorithm_const_sptr alg);
    void observeFinish(IAlgorithm_const_sptr alg);
    void observeError(IAlgorithm_const_sptr alg);

    void stopObserving(IAlgorithm_const_sptr alg);
    void stopObservingManager();

    virtual void progressHandle(const IAlgorithm* alg,double p,const std::string& msg);
    virtual void startingHandle(IAlgorithm_sptr alg);
    virtual void startHandle(const IAlgorithm* alg);
    virtual void finishHandle(const IAlgorithm* alg);
    virtual void errorHandle(const IAlgorithm* alg,const std::string& what);

private:
    void _progressHandle(const Poco::AutoPtr<Algorithm::ProgressNotification>& pNf);
    /// Poco::NObserver for Algorithm::ProgressNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::ProgressNotification> m_progressObserver;

    void _startHandle(const Poco::AutoPtr<Algorithm::StartedNotification>& pNf);
    /// Poco::NObserver for Algorithm::StartedNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::StartedNotification> m_startObserver;

    void _finishHandle(const Poco::AutoPtr<Algorithm::FinishedNotification>& pNf);
    /// Poco::NObserver for Algorithm::FinishedNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::FinishedNotification> m_finishObserver;

    void _errorHandle(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf);
    /// Poco::NObserver for Algorithm::ErrorNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::ErrorNotification> m_errorObserver;

    void _startingHandle(const Poco::AutoPtr<AlgorithmStartingNotification>& pNf);
    /// Poco::NObserver for API::AlgorithmStartingNotification
    Poco::NObserver<AlgorithmObserver, AlgorithmStartingNotification> m_startingObserver;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMOBSERVER_H_*/
