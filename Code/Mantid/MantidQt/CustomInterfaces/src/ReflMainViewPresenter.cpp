#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    using namespace Mantid::API;

    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): m_model(model), m_view(view)
    {
    }

    ReflMainViewPresenter::ReflMainViewPresenter(std::string model, ReflMainView* view): m_view(view)
    {
      m_model = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(model);
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {
    }

    void ReflMainViewPresenter::load()
    {
      m_view->showTable(m_model);
    }
  }
}