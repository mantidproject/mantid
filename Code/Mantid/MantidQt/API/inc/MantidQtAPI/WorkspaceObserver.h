#ifndef WORKSPACE_OBSERVER_H
#define WORKSPACE_OBSERVER_H

#include "MantidAPI/AnalysisDataService.h"
#include <Poco/NObserver.h>
#include <QObject>
#include "DllOption.h"

//------------------------------------------------
// Mantid Forward declaration
//------------------------------------------------

namespace MantidQt
{
  namespace API
  {
    //------------------------------------------------
    // Forward declaration
    //------------------------------------------------
    class WorkspaceObserver;

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
    class EXPORT_OPT_MANTIDQT_API ObserverCallback : public QObject
    {
      Q_OBJECT

    public:
      ObserverCallback(WorkspaceObserver *observer) : QObject(NULL), m_observer(observer)
      {
      }

    signals:
      /// Delete signal handler
      void preDeleteRequested(const std::string &name, Mantid::API::Workspace_sptr workspace);
      void postDeleteRequested(const std::string &name);
      void addRequested(const std::string &name, Mantid::API::Workspace_sptr workspace);
      void afterReplaced(const std::string &name, Mantid::API::Workspace_sptr workspace);
      void renamed(const std::string &oldName, const std::string &newName);
      void adsCleared();

    private slots:
      /// Pre Delete slot
      void handlePreDelete(const std::string &name, Mantid::API::Workspace_sptr workspace);
      /// Post Delete slot
      void handlePostDelete(const std::string &name);
      /// Add slot
      void handleAdd(const std::string &name,  Mantid::API::Workspace_sptr workspace);
      /// Replace slot
      void handleAfterReplace(const std::string &name,  Mantid::API::Workspace_sptr workspace);
      /// Rename slot
      void handleRename(const std::string &oldName, const std::string &newName);
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTIDQT_API WorkspaceObserver
    {

    public:
      /// Default constructor
      WorkspaceObserver();
      /// Destructor
      virtual ~WorkspaceObserver();
      /// Observe workspace deletes
      void observePreDelete(bool on = true);
      /// Observe workspace deletes
      void observePostDelete(bool on = true);
      /// Observe replacements
      void observeAfterReplace(bool on = true);
      /// Observe renaming
      void observeRename(bool on = true);
      /// Observe additions
      void observeAdd(bool on = true);
      /// Observe clearances
      void observeADSClear(bool on = true);

    protected:
      /** Handler of the delete notifications. Could be overriden in inherited classes.
      The default handler is provided (doing nothing).
      @param wsName :: The name of the deleted workspace.
      @param ws :: Pointer to the workspace to be deleted.
      */
      virtual void preDeleteHandle(const std::string& wsName, const Mantid::API::Workspace_sptr ws)
      {
        Q_UNUSED(wsName);
        Q_UNUSED(ws);
      }

      /** Handler of the delete notifications. Could be overriden in inherited classes.
      The default handler is provided (doing nothing).
      @param wsName :: The name of the deleted workspace.
      */
      virtual void postDeleteHandle(const std::string& wsName)
      {
        Q_UNUSED(wsName);
      }
      /** Handler of the add notifications. Could be overriden in inherited classes.
      The default handler is provided (doing nothing).
      @param wsName :: The name of the added workspace.
      @param ws :: The shared pointer to the workspace to be added.
      */
      virtual void addHandle(const std::string& wsName,
        const Mantid::API::Workspace_sptr ws)
      {
        Q_UNUSED(wsName);
        Q_UNUSED(ws);
      }
      /** Handler of the AfterReplace notifications. Could be overriden in inherited classes.
      The default handler is provided (doing nothing).
      @param wsName :: The name of the deleted workspace.
      @param ws :: The shared pointer to the workspace to be deleted.
      */
      virtual void afterReplaceHandle(const std::string& wsName,
        const Mantid::API::Workspace_sptr ws)
      {
        Q_UNUSED(wsName);
        Q_UNUSED(ws);
      }

      /** Handler of the Rename notifications. Could be overriden in inherited classes.
      The default handler is provided (doing nothing).
      @param oldName :: The old name of a workspace
      @param newName :: The new name of a workspace
      */
      virtual void renameHandle(const std::string& oldName,
        const std::string& newName)
      {
        Q_UNUSED(oldName);
        Q_UNUSED(newName);
      }

      /** Handle an ADS clear notification
      * 
      */
      virtual void clearADSHandle()
      {
      }

    protected:
      /** Poco notification handler for DataService::PostDeleteNotification.
      @param pNf :: The pointer to the notification.
      */
      void _preDeleteHandle(Mantid::API::WorkspacePreDeleteNotification_ptr pNf)
      {
        m_proxy->preDeleteRequested(pNf->objectName(), pNf->object());
      }
      /// Poco::NObserver for DataServise::DeleteNotification.
      Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspacePreDeleteNotification> m_preDeleteObserver;

      /** Poco notification handler for DataService::PostDeleteNotification.
      @param pNf :: The pointer to the notification.
      */
      void _postDeleteHandle(Mantid::API::WorkspacePostDeleteNotification_ptr pNf)
      {
        m_proxy->postDeleteRequested(pNf->objectName());
      }
      /// Poco::NObserver for DataServise::DeleteNotification.
      Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspacePostDeleteNotification> m_postDeleteObserver;

      /** Poco notification handler for DataService::DeleteNotification.
      @param pNf :: The pointer to the notification.
      */
      void _addHandle(Mantid::API::WorkspaceAddNotification_ptr pNf)
      {
        m_proxy->addRequested(pNf->objectName(), pNf->object());
      }
      /// Poco::NObserver for DataServise::AddNotification.
      Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceAddNotification> m_addObserver;

      /** Poco notification handler for DataService::AfterReplaceNotification.
      @param pNf :: The pointer to the notification.
      */
      void _afterReplaceHandle(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf)
      {
        m_proxy->afterReplaced(pNf->objectName(), pNf->object());
      }
      /// Poco::NObserver for DataServise::AfterReplaceNotification.
      Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceAfterReplaceNotification> m_afterReplaceObserver;

      /** Poco notification handler for DataService::RenameNotification.
      @param pNf :: The pointer to the notification.
      */
      void _renameHandle(Mantid::API::WorkspaceRenameNotification_ptr pNf)
      {
        m_proxy->renamed(pNf->objectName(), pNf->newObjectName());
      }
      /// Poco::NObserver for DataServise::RenameNotification.
      Poco::NObserver<WorkspaceObserver, Mantid::API::WorkspaceRenameNotification> m_renameObserver;

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

      bool m_predel_observed, m_postdel_observed, m_add_observed, m_repl_observed, m_rename_observed, m_clr_observed;
    };


  } // MantidQt
} // API

#endif
