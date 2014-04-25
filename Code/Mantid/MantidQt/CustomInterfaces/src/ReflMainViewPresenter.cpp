#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    using namespace Mantid::API;

    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view)
    {
      view->showTable(model);
    }

    ReflMainViewPresenter::ReflMainViewPresenter(std::string model, ReflMainView* view)
    {
      ITableWorkspace_sptr tws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(model);
      view->showTable(tws);
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {
    }

    void ReflMainViewPresenter::load()
    {
    }
  }
}