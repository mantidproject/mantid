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

    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): m_view(view), m_model(model), m_cache_name("")
    {
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {

    }

    void ReflMainViewPresenter::process()
    {

    }

    void ReflMainViewPresenter::addRow()
    {
      m_model->appendRow();
    }

    void ReflMainViewPresenter::notify()
    {
      if(m_view->getSaveAsFlag())
      {
        saveAs();
      }
      else if(m_view->getSaveFlag())
      {
        save();
      }
      else if(m_view->getAddRowFlag())
      {
        addRow();
      }
      else if(m_view->getProcessFlag())
      {
        process();
      }
      else if(m_view->getDeleteRowFlag())
      {

      }

      m_view->clearNotifyFlags();
    }

    void ReflMainViewPresenter::load()
    {
      m_view->showTable(m_model);
    }
  }
}