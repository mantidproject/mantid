#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    ReflMainViewPresenter::ReflMainViewPresenter(Mantid::API::ITableWorkspace_sptr model, ReflMainView* view)
    {
      view->showTable(model);
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {
    }
  }
}