#include "MantidQtMantidWidgets/ICatMyDataSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

#include<QStringList>
#include<QTreeWidget>
#include<QTreeWidgetItem>
#include<QFont>

using namespace Mantid::API;

namespace MantidQt
{
  namespace MantidWidgets
  {

    ICatMyDataSearch::ICatMyDataSearch(QWidget*par):QWidget(par),m_utils_sptr(new ICatUtils)
    {
      m_uiForm.setupUi(this);

      connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writeErrorToLogWindow(const QString&)));
      connect(m_uiForm.myDatatableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem* )),
          this,SLOT(investigationSelected(QTableWidgetItem* )));

      QObject* qobj=parent();
      QWidget* parent=qobject_cast<QWidget*>(qobj->parent());
      if(parent)
      {
        setparentWidget(parent);
      }
      if(!m_utils_sptr)
      {
        return;
      }
      m_utils_sptr->setParent(parent);
      Mantid::API::ITableWorkspace_sptr  ws_sptr ;
      if(executeMyDataSearch(ws_sptr))
      {
        m_utils_sptr->updatesearchResults(ws_sptr,m_uiForm.myDatatableWidget);
        m_utils_sptr->updateSearchLabel(ws_sptr,m_uiForm.mydatalabel);
      }
    }

    /* this method sets the parent widget as application window
     */
    void ICatMyDataSearch::setparentWidget(QWidget* par)
    {
      m_applicationWindow= par;
    }
    bool ICatMyDataSearch::executeMyDataSearch(ITableWorkspace_sptr& ws_sptr)
    {
      Mantid::API::IAlgorithm_sptr alg;
      try
      {
        alg = Mantid::API::AlgorithmManager::Instance().create("CatalogMyDataSearch",1);
      }
      catch(...)
      {
        throw std::runtime_error("Error when loading Mydata search results.");
      }
      try
      {
        alg->setPropertyValue("OutputWorkspace","MyInvestigations");

      }
      catch(std::invalid_argument& e)
      {
        emit error(e.what());
        return false;
      }
      catch (Mantid::Kernel::Exception::NotFoundError& e)
      {
        emit error(e.what());
        return false;
      }


      Poco::ActiveResult<bool> result(alg->executeAsync());
      while( !result.available() )
      {
        QCoreApplication::processEvents();
      }
      if(result.failed())
      {
        if(!m_utils_sptr)
        {
          return false;
        }
        //if the algorithm failed check the session id passed is valid
        if(!m_utils_sptr->isSessionValid(alg))
        {
          //at this point session is invalid, popup loginbox to login
          if(m_utils_sptr->login())
          {

            return(executeMyDataSearch(ws_sptr)?true:false );

          }
          else
          {
            return false;
          }

        }
      }

      if(AnalysisDataService::Instance().doesExist("MyInvestigations"))
      {
        ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
        (AnalysisDataService::Instance().retrieve("MyInvestigations"));

      }

      return true;

    }
    void ICatMyDataSearch::investigationSelected(QTableWidgetItem* item)
    {
      ICatUtils utils;
      utils.investigationSelected(m_uiForm.myDatatableWidget,item,m_applicationWindow,m_ws2_sptr);
    }

  }
}

