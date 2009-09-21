#ifndef WORKSPACE_OBSERVER_H
#define WORKSPACE_OBSERVER_H

#include "MantidAPI/AnalysisDataService.h"
#include <Poco/NObserver.h>

/** @class WorkspaceObserver 

 Observes AnalysisDataService notifications: add,replace,delete.
 Hides Poco::Notification API from the user. 

 @author Roman Tolchenov, Tessella plc
 @date 18/09/2009

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

class WorkspaceObserver
{
public:
  WorkspaceObserver()
    :m_deleteObserver(*this,&WorkspaceObserver::_deleteHandle),
    m_afterReplaceObserver(*this,&WorkspaceObserver::_afterReplaceHandle)
  {}

  ~WorkspaceObserver()
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_afterReplaceObserver);
  }

  void observeDelete()
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
  }

  void observeAfterReplace()
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_afterReplaceObserver);
  }

protected:
    /** Handler of the delete notifications. Could be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param wsName The name of the deleted workspace.
        @param ws The shared pointer to the workspace to be deleted.
    */
  virtual void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
  {
  }
    /** Handler of the AfterReplace notifications. Could be overriden in inherited classes.
        The default handler is provided (doing nothing).
        @param wsName The name of the deleted workspace.
        @param ws The shared pointer to the workspace to be deleted.
    */
  virtual void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
  {
  }
private:

  /** Poco notification handler for DataService::DeleteNotification.
  @param pNf The pointer to the notification.
  */
  void _deleteHandle(Mantid::API::WorkspaceDeleteNotification_ptr pNf)
  {
    this->deleteHandle(pNf->object_name(),pNf->object());
  }
  /// Poco::NObserver for DataServise::DeleteNotification.
  Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceDeleteNotification> m_deleteObserver;

  /** Poco notification handler for DataService::AfterReplaceNotification.
  @param pNf The pointer to the notification.
  */
  void _afterReplaceHandle(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf)
  {
    this->afterReplaceHandle(pNf->object_name(),pNf->object());
  }
  /// Poco::NObserver for DataServise::DeleteNotification.
  Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceAfterReplaceNotification> m_afterReplaceObserver;

};

#endif
