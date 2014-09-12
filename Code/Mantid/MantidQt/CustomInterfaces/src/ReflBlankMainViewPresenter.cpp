#include "MantidQtCustomInterfaces/ReflBlankMainViewPresenter.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
using namespace Mantid::API;
namespace
{
  ITableWorkspace_sptr createWorkspace()
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("str","Scale");
    auto colStitch = ws->addColumn("int","StitchGroup");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);

    TableRow row = ws->appendRow();
    return ws;
  }
}

namespace MantidQt
{
  namespace CustomInterfaces
  {


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    ReflBlankMainViewPresenter::ReflBlankMainViewPresenter(ReflMainView* view): ReflMainViewPresenter(view)
    {
      m_model = createWorkspace();
      load();
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflBlankMainViewPresenter::~ReflBlankMainViewPresenter()
    {
    }

    /**
    Press changes to a previously saved-to item in the ADS, or ask for a name if never given one
    */
    void ReflBlankMainViewPresenter::save()
    {
      if (m_cache_name != "")
      {
        AnalysisDataService::Instance().addOrReplace(m_cache_name, boost::shared_ptr<ITableWorkspace>(m_model->clone()));
      }
      else
      {
        saveAs();
      }
    }

    /**
    Press changes to a new item in the ADS
    */
    void ReflBlankMainViewPresenter::saveAs()
    {
      if(m_view->askUserString("Save As", "Enter a workspace name:", "Workspace"))
      {
        m_cache_name = m_view->getUserString();
        save();
      }
    }
  } // namespace CustomInterfaces
} // namespace Mantid
