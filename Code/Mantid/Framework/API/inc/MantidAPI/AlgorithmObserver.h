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

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    /// Default constructor. Notification hanlders are not connected to any algorithm
    AlgorithmObserver()
        :m_progressObserver(*this,&AlgorithmObserver::_progressHandle),
        m_startObserver(*this,&AlgorithmObserver::_startHandle),
        m_finishObserver(*this,&AlgorithmObserver::_finishHandle),
        m_errorObserver(*this,&AlgorithmObserver::_errorHandle)
    {
    }
    /**   Constructs AlgorithmObserver and connects all its handlers to algorithm alg.
          @param alg :: Algorithm to be observed
      */
    AlgorithmObserver(IAlgorithm_const_sptr alg)
        :m_progressObserver(*this,&AlgorithmObserver::_progressHandle),
        m_startObserver(*this,&AlgorithmObserver::_startHandle),
        m_finishObserver(*this,&AlgorithmObserver::_finishHandle),
        m_errorObserver(*this,&AlgorithmObserver::_errorHandle)
    {
        observeAll(alg);
    }
    
    virtual ~AlgorithmObserver() {}
    

    /**   Connect to algorithm alg and observe all its notifications
          @param alg :: Algorithm to be observed
    */
    void observeAll(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_progressObserver);
        alg->addObserver(m_startObserver);
        alg->addObserver(m_finishObserver);
        alg->addObserver(m_errorObserver);
    }

    /**   Connect to algorithm alg and observe its progress notification
          @param alg :: Algorithm to be observed
    */
    void observeProgress(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_progressObserver);
    }

    /**   Connect to algorithm alg and observe its start notification
          @param alg :: Algorithm to be observed
    */
    void observeStart(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_startObserver);
    }

    /**   Connect to algorithm alg and observe its finish notification
          @param alg :: Algorithm to be observed
    */
    void observeFinish(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_finishObserver);
    }

    /**   Connect to algorithm alg and observe its error notification
          @param alg :: Algorithm to be observed
    */
    void observeError(IAlgorithm_const_sptr alg)
    {
        alg->addObserver(m_errorObserver);
    }

    /**   Disconnect from algorithm alg. Should be called in the destructor of inherited classes.
          @param alg :: Algorithm to be disconnected
    */
    void stopObserving(IAlgorithm_const_sptr alg)
    {
        alg->removeObserver(m_progressObserver);
        alg->removeObserver(m_startObserver);
        alg->removeObserver(m_finishObserver);
        alg->removeObserver(m_errorObserver);
    }

/// @cond Doxygen cannot handle the macro around the argument name

    /** Handler of the progress notifications. Must be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param alg :: Pointer to the algorithm sending the notification. Note that this can
        point to a different object than the argument of a observeZZZ(...) method, e.g. 
        an observer can be connected to an AlgorithmProxy instance and receive notifications from
        the corresponding Algorithm type object.
        @param p :: Progress reported by the algorithm, 0 <= p <= 1
        @param msg :: Optional message string sent by the algorithm
    */
    virtual void progressHandle(const IAlgorithm* alg,double p,const std::string& msg)
    {
      (void*)alg;
      (void)msg;
      std::cerr<<"Progress "<<p<<'\n';
    }

    /** Handler of the start notifications. Must be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param alg :: Pointer to the algorithm sending the notification. Note that this can
        point to a different object than the argument of a observeZZZ(...) method, e.g. 
        an observer can be connected to an AlgorithmProxy instance and receive notifications from
        the corresponding Algorithm type object.
    */
    virtual void startHandle(const IAlgorithm* alg)
    {
      (void*)alg;
      std::cerr<<"Started "<<'\n';
    }
   /** Handler of the finish notifications. Must be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param alg :: Pointer to the algorithm sending the notification. Note that this can
        point to a different object than the argument of a observeZZZ(...) method, e.g. 
        an observer can be connected to an AlgorithmProxy instance and receive notifications from
        the corresponding Algorithm type object.
    */
    virtual void finishHandle(const IAlgorithm* alg)
    {
      (void*)alg;
      std::cerr<<"Finished "<<'\n';
    }
    /** Handler of the error notifications. Must be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param alg :: Pointer to the algorithm sending the notification. Note that this can
        point to a different object than the argument of a observeZZZ(...) method, e.g. 
        an observer can be connected to an AlgorithmProxy instance and receive notifications from
        the corresponding Algorithm type object.
        @param what :: The error message
    */
    virtual void errorHandle(const IAlgorithm* alg,const std::string& what)
    {
      (void*)alg;
      std::cerr<<"Error "<<what<<'\n';
    }
/// @endcond
private:

    /** Poco notification handler for Algorithm::ProgressNotification.
        @param pNf :: An pointer to the notification.
    */
    void _progressHandle(const Poco::AutoPtr<Algorithm::ProgressNotification>& pNf)
    {
        this->progressHandle(pNf->algorithm(),pNf->progress,pNf->message);
    }
    /// Poco::NObserver for Algorithm::ProgressNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::ProgressNotification> m_progressObserver;

    /** Poco notification handler for Algorithm::StartedNotification.
        @param pNf :: An pointer to the notification.
    */
    void _startHandle(const Poco::AutoPtr<Algorithm::StartedNotification>& pNf)
    {
        this->startHandle(pNf->algorithm());
    }
    /// Poco::NObserver for Algorithm::StartedNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::StartedNotification> m_startObserver;

    /** Poco notification handler for Algorithm::FinishedNotification.
        @param pNf :: An pointer to the notification.
    */
    void _finishHandle(const Poco::AutoPtr<Algorithm::FinishedNotification>& pNf)
    {
        this->finishHandle(pNf->algorithm());
    }
    /// Poco::NObserver for Algorithm::FinishedNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::FinishedNotification> m_finishObserver;

    /** Poco notification handler for Algorithm::ErrorNotification.
        @param pNf :: An pointer to the notification.
    */
    void _errorHandle(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf)
    {
        this->errorHandle(pNf->algorithm(),pNf->what);
    }
    /// Poco::NObserver for Algorithm::ErrorNotification.
    Poco::NObserver<AlgorithmObserver, Algorithm::ErrorNotification> m_errorObserver;

};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHMOBSERVER_H_*/
