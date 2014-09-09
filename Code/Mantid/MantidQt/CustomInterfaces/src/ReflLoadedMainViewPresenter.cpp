#include "MantidQtCustomInterfaces/ReflLoadedMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
using namespace Mantid::API;
namespace
{
  void hasValidModel(ITableWorkspace_sptr model)
  {
    if(model->columnCount() != 8)
      throw std::runtime_error("Selected table has the incorrect number of columns (8) to be used as a reflectometry table.");

    try
    {
      model->String(0,0);
      model->String(0,1);
      model->String(0,2);
      model->String(0,3);
      model->String(0,4);
      model->String(0,5);
      model->String(0,6);
      model->Int(0,7);
    }
    catch(const std::runtime_error&)
    {
      throw std::runtime_error("Selected table does not meet the specifications to become a model for this interface.");
    }
  }
}
namespace MantidQt
{
  namespace CustomInterfaces
  {


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */

    ReflLoadedMainViewPresenter::ReflLoadedMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view):
      ReflMainViewPresenter(boost::shared_ptr<ITableWorkspace>(model->clone()), view)
    {
      if (model->name() != "")
      {
        m_cache_name = model->name();
      }
      else
      {
        throw std::runtime_error("Supplied model workspace must have a name");
      }
      m_cache = model;
      hasValidModel(m_model);
      load();
    }

    ReflLoadedMainViewPresenter::ReflLoadedMainViewPresenter(std::string model, ReflMainView* view):
      ReflMainViewPresenter(boost::shared_ptr<ITableWorkspace>(AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(model)->clone()), view)
    {
      m_cache = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(model);
      if (m_cache->name() != "")
      {
        m_cache_name = m_cache->name();
      }
      else
      {
        throw std::runtime_error("Supplied model workspace must have a name");
      }
      hasValidModel(m_model);
      load();
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflLoadedMainViewPresenter::~ReflLoadedMainViewPresenter()
    {

    }

    /**
    Press changes to the same item in the ADS
    */
    void ReflLoadedMainViewPresenter::save()
    {
      AnalysisDataService::Instance().addOrReplace(m_cache_name,boost::shared_ptr<ITableWorkspace>(m_model->clone()));
    }

    /**
    Press changes to a new item in the ADS
    */
    void ReflLoadedMainViewPresenter::saveAs()
    {
      if(m_view->askUserString("Save As", "Enter a workspace name:", "Workspace"))
      {
        m_cache_name = m_view->getUserString();
        save();
      }
    }

  } // namespace CustomInterfaces
} // namespace Mantid
