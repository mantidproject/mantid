#ifndef MANTID_KERNEL_ALGORITHMOBSERVER_H_
#define MANTID_KERNEL_ALGORITHMOBSERVER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include <boost/shared_ptr.hpp>
#include <Poco/ActiveMethod.h>
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>

namespace Mantid
{
namespace API
{

/** @class AlgorithmObserver AlgorithmObserver.h API/AlgorithmObserver.h

 Observes Algorithm notifications: start,progress,finish,error.
 Hides Poco::Notification API from the user. 

 @author Roman Tolchenov, Tessella Support Services plc
 @date 09/02/2009

 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport AlgorithmObserver
{
public:
    AlgorithmObserver()
        :m_progressObserver(*this,&AlgorithmObserver::_progressHandle),
        m_startObserver(*this,&AlgorithmObserver::_startHandle),
        m_finishObserver(*this,&AlgorithmObserver::_finishHandle),
        m_errorObserver(*this,&AlgorithmObserver::_errorHandle)
    {
    }
    AlgorithmObserver(IAlgorithm_const_sptr alg)
        :m_progressObserver(*this,&AlgorithmObserver::_progressHandle),
        m_startObserver(*this,&AlgorithmObserver::_startHandle),
        m_finishObserver(*this,&AlgorithmObserver::_finishHandle),
        m_errorObserver(*this,&AlgorithmObserver::_errorHandle)
    {
        observeAll(alg);
    }
    void observeAll(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_progressObserver);
        alg->addObserver(m_startObserver);
        alg->addObserver(m_finishObserver);
        alg->addObserver(m_errorObserver);
    }

    void observeProgress(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_progressObserver);
    }

    void observeStart(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_startObserver);
    }

    void observeFinish(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_finishObserver);
    }

    void observeError(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_errorObserver);
    }

    // Handling progress notifications
    virtual void progressHandle(const IAlgorithm* alg,double p,const std::string& msg)
    {
        std::cerr<<"Progress "<<p<<'\n';
    }
    void _progressHandle(const Poco::AutoPtr<Algorithm::ProgressNotification>& pNf)
    {
        this->progressHandle(pNf->algorithm(),pNf->progress,pNf->message);
    }
    Poco::NObserver<AlgorithmObserver, Algorithm::ProgressNotification> m_progressObserver;

    // Handling start notifications
    virtual void startHandle(const IAlgorithm* alg)
    {
        std::cerr<<"Started "<<'\n';
    }
    void _startHandle(const Poco::AutoPtr<Algorithm::StartedNotification>& pNf)
    {
        this->startHandle(pNf->algorithm());
    }
    Poco::NObserver<AlgorithmObserver, Algorithm::StartedNotification> m_startObserver;

    // Handling finish notifications
    virtual void finishHandle(const IAlgorithm* alg)
    {
        std::cerr<<"Finished "<<'\n';
    }
    void _finishHandle(const Poco::AutoPtr<Algorithm::FinishedNotification>& pNf)
    {
        this->finishHandle(pNf->algorithm());
    }
    Poco::NObserver<AlgorithmObserver, Algorithm::FinishedNotification> m_finishObserver;

    // Handling error notifications
    virtual void errorHandle(const IAlgorithm* alg,const std::string& what)
    {
        std::cerr<<"Error "<<what<<'\n';
    }
    void _errorHandle(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf)
    {
        this->errorHandle(pNf->algorithm(),pNf->what);
    }
    Poco::NObserver<AlgorithmObserver, Algorithm::ErrorNotification> m_errorObserver;

};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHMOBSERVER_H_*/
