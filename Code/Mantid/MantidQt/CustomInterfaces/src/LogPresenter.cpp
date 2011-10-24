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
    LogPresenter::LogPresenter(LoanedMemento& memento) : m_WsMemento(memento), m_view(NULL)
    {

    }

    /// Destructor
    LogPresenter::~LogPresenter()
    {
    }

    /**
    Update the presenter. Trigger to read from view and calculate some action to perform.
    */
    void LogPresenter::update()
    {
      std::map<std::string, std::string> logValues = m_view->getLogData();

      WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
      std::vector<AbstractMementoItem_sptr> existingLogData = service.getLogData();
      size_t proposedSize = logValues.size();
      size_t existingSize = existingLogData.size();
      LogDataMap::iterator it = logValues.begin();
      size_t count = 0;
      std::vector<std::string> newLogValues(proposedSize);
      while(it != logValues.end())
      {
        if(count >= existingSize)
        {
          service.addLogItem(it->first); //Expand as necessary
        }
        newLogValues[count] = it->second; //Overrite values.
        count++;
        it++;
      } 
      service.setLogData(newLogValues);
    }

    /**
    Accept the MVP view
    @param : view to track.
    */
    void LogPresenter::acceptView(LogView* view)
    {
      m_view = view;
      WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
      view->initalize(service.getLogData());
    }
  }
}