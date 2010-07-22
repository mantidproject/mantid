#ifndef WORKSPACE_OBSERVER_H
#define WORKSPACE_OBSERVER_H

#include "MantidAPI/AnalysisDataService.h"
#include <Poco/NObserver.h>
#include <QObject>

//------------------------------------------------
// Forward declaration
//------------------------------------------------
class WorkspaceObserver;

#ifndef IGNORE_WORKSPACE_OBSERVER_ARGUMENT
#define IGNORE_WORKSPACE_OBSERVER_ARGUMENT(x)
#endif
/**
 * A simple callback class so that we avoid multiple inheritance issues with QObject.
 * 
 * This adds an extra level of indirection to the call between a Poco notification handler and the call to the correct WorkspaceObserver handler.
 * It is necessary to do this rather than just call the function directly so that the function call gets executed in the object's thread rather than
 * in the thread that the notificiation was recieved in.
 *
 * Multiple inheritance is not used in WorkspaceObserver as their seems to be some problem using it and QObject
 * 
 */
class ObserverCallback : public QObject
{
  Q_OBJECT
  
public:
  ObserverCallback(WorkspaceObserver *observer) : QObject(NULL), m_observer(observer)
  {
  }

signals:
  /// Delete signal handler
  void deleteRequested(const std::string &name, Mantid::API::Workspace_sptr workspace);
  void afterReplaced(const std::string &name, Mantid::API::Workspace_sptr workspace);
  void adsCleared();
  
private slots:
  /// Delete slot
  void handleDelete(const std::string &name,  Mantid::API::Workspace_sptr workspace);
  /// Replace slot
  void handleAfterReplace(const std::string &name,  Mantid::API::Workspace_sptr workspace);
  ///Clear slot
  void handleClearADS();

private:
  friend class WorkspaceObserver;
  /// Default constructor
  ObserverCallback();
  /// Object to call back to
  WorkspaceObserver *m_observer;
};

/** @class WorkspaceObserver 

 Observes AnalysisDataService notifications: add,replace,delete.
 Hides Poco::Notification API from the user. 

 @author Roman Tolchenov, Tessella plc
 @date 18/09/2009

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
class WorkspaceObserver
{
public:
  /// Default constructor
  WorkspaceObserver() :
    m_deleteObserver(*this,&WorkspaceObserver::_deleteHandle),
    m_afterReplaceObserver(*this,&WorkspaceObserver::_afterReplaceHandle),
    m_clearADSObserver(*this,&WorkspaceObserver::_clearADSHandle)
  {
    m_proxy = new ObserverCallback(this);
  }

  /// Destructor
  virtual ~WorkspaceObserver()
  {
    delete m_proxy;

    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_afterReplaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_clearADSObserver);
  }

  void observeDelete()
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
    m_proxy->connect(m_proxy, 
		     SIGNAL(deleteRequested(const std::string &,Mantid::API::Workspace_sptr)),
		     SLOT(handleDelete(const std::string &, Mantid::API::Workspace_sptr)),
		     Qt::QueuedConnection);
  }

  void observeAfterReplace()
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_afterReplaceObserver);
    m_proxy->connect(m_proxy, 
		     SIGNAL(afterReplaced(const std::string &,Mantid::API::Workspace_sptr)),
		     SLOT(handleAfterReplace(const std::string &, Mantid::API::Workspace_sptr)),
		     Qt::QueuedConnection
		     );
  }

  void observeADSClear()
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_clearADSObserver);
    m_proxy->connect(m_proxy,SIGNAL(adsCleared()), SLOT(handleClearADS()), Qt::QueuedConnection);
  }

protected:
    /** Handler of the delete notifications. Could be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param wsName The name of the deleted workspace.
        @param ws The shared pointer to the workspace to be deleted.
    */
  virtual void deleteHandle(const std::string& IGNORE_WORKSPACE_OBSERVER_ARGUMENT(wsName),
                            const boost::shared_ptr<Mantid::API::Workspace> IGNORE_WORKSPACE_OBSERVER_ARGUMENT(ws))
  {
  }
    /** Handler of the AfterReplace notifications. Could be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param wsName The name of the deleted workspace.
        @param ws The shared pointer to the workspace to be deleted.
    */
  virtual void afterReplaceHandle(const std::string& IGNORE_WORKSPACE_OBSERVER_ARGUMENT(wsName),
                                  const boost::shared_ptr<Mantid::API::Workspace> IGNORE_WORKSPACE_OBSERVER_ARGUMENT(ws))
  {
  }

  /** Handle an ADS clear notification
   * 
   */
  virtual void clearADSHandle()
  {
  }

protected:
  /** Poco notification handler for DataService::DeleteNotification.
  @param pNf The pointer to the notification.
  */
  void _deleteHandle(Mantid::API::WorkspaceDeleteNotification_ptr pNf)
  {
    m_proxy->deleteRequested(pNf->object_name(), pNf->object());
  }
  /// Poco::NObserver for DataServise::DeleteNotification.
  Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceDeleteNotification> m_deleteObserver;

  /** Poco notification handler for DataService::AfterReplaceNotification.
  @param pNf The pointer to the notification.
  */
  void _afterReplaceHandle(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf)
  {
    m_proxy->afterReplaced(pNf->object_name(), pNf->object());
  }
  /// Poco::NObserver for DataServise::DeleteNotification.
  Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceAfterReplaceNotification> m_afterReplaceObserver;
  
  ///Clear notification observer
  Poco::NObserver<WorkspaceObserver, Mantid::API::ClearADSNotification> m_clearADSObserver;
  /// ADS clear notification
  void _clearADSHandle(Mantid::API::ClearADSNotification_ptr)
  {
    m_proxy->adsCleared();
  }

private:
  friend class ObserverCallback;
  ObserverCallback *m_proxy;
};


#endif
