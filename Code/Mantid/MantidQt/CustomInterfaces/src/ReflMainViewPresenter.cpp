#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
using namespace Mantid::API;

namespace
{
  void hasValidModel(ITableWorkspace_sptr model)
  {
    std::runtime_error invalid("Selected table does not meet the specifications to become a model for this interface.");
    //TODO: wrap in try-catch
    if (model->columnCount() == 8)
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
    else
    {
      throw invalid;
    }
  }
}

namespace MantidQt
{
  namespace CustomInterfaces
  {
    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): m_model(model), m_view(view)
    {
      hasValidModel(m_model);
      load();
    }

    ReflMainViewPresenter::ReflMainViewPresenter(std::string model, ReflMainView* view): m_model(AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(model)), m_view(view)
    {
      hasValidModel(m_model);
      load();
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {

    }

    void ReflMainViewPresenter::notify()
    {
    }

    void ReflMainViewPresenter::load()
    {
      m_view->showTable(m_model);
    }
  }
}