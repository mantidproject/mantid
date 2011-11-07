#include "MantidQtCustomInterfaces/LogPresenter.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/LogView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Constructor
    LogPresenter::LogPresenter(LoanedMemento& memento) : m_WsMemento(memento), m_readOnlyView(NULL), m_editableView(NULL), m_editableMode(false)
    {

    }

    /// Destructor
    LogPresenter::~LogPresenter()
    {
    }

    /// Helper method to switch the current view between read-only and editable.
    void LogPresenter::swapViews()
    {
      if(m_editableMode)
      {
        m_currentView = m_readOnlyView;
        m_editableView->hide();
        m_readOnlyView->show();
        m_editableMode = false;
      }
      else
      {
        m_currentView = m_editableView;
        m_editableView->show();
        m_readOnlyView->hide();
        m_editableMode = true;
      }
      //Ensure that displays on both views are kept synchronised.
      WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
      m_currentView->initalize(service.getLogData());
    }

    /**
    Update the presenter. Trigger to read from view and calculate some action to perform.
    */
    void LogPresenter::update()
    {
      //Do some validation checks.
      if(NULL == m_editableView)
      {
        throw std::runtime_error("A editable view has not been provided for this LogPresenter");
      }
      if(NULL == m_readOnlyView)
      {
        throw std::runtime_error("A read-only view has not been provided for this LogPresenter");
      }

      LogViewStatus viewStatus = m_currentView->fetchStatus();
      WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
      std::vector<AbstractMementoItem_sptr> existingLogData = service.getLogData();

      //Swap the views.
      if(LogViewStatus::switching_mode == viewStatus)
      {
        swapViews();
      }
      else if(LogViewStatus::cancelling == viewStatus)
      {
        std::vector<AbstractMementoItem_sptr>::iterator it = existingLogData.begin();
        while(it != existingLogData.end())
        {
          (*it)->rollback(); //Roll back changes to logs.
          it++;
        }
        swapViews();
      }
      else if(LogViewStatus::saving == viewStatus)
      {
        LogDataMap logValues = m_currentView->getLogData();
        size_t proposedSize = logValues.size();
        size_t existingSize = existingLogData.size();
        LogDataMap::iterator it = logValues.begin();
        size_t count = 0;
        std::vector<std::string> newLogValues(proposedSize);
        while(it != logValues.end())
        {
          if(count >= existingSize)
          {
            service.declareLogItem(it->first); //Expand as necessary
          }
          newLogValues[count] = it->second; //Overrite values.
          count++;
          it++;
        }
        service.setLogData(newLogValues);
        swapViews();
      }
    }

    /**
    Accept the MVP view
    @param : view to track.
    */
    void LogPresenter::acceptReadOnlyView(LogView* view)
    {
      m_readOnlyView = view;
      WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
      m_readOnlyView->initalize(service.getLogData());
      m_readOnlyView->show();
      m_currentView = m_readOnlyView; //By default, current view is read-only.
    }

    /**
    Accept the MVP view
    @param : view to track.
    */
    void LogPresenter::acceptEditableView(LogView* view)
    {
      m_editableView = view;
      WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
      m_editableView->initalize(service.getLogData());
      m_editableView->hide();
    }


  }
}