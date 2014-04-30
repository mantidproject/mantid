#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    void ReflNullMainViewPresenter::notify()
    {
      throw std::runtime_error("Cannot notify a null presenter");
    }

    ReflNullMainViewPresenter::~ReflNullMainViewPresenter()
    {
    }
  }
}