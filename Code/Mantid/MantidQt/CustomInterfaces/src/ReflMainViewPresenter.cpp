#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {

    ReflMainViewPresenter::ReflMainViewPresenter(ReflMainView* view): m_view(view)
    {
    }

    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): m_view(view), m_model(model)
    {
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