#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    void ReflNullMainViewPresenter::load()
    {
      throw std::runtime_error("Cannot load from a null presenter");
    }

    ReflNullMainViewPresenter::~ReflNullMainViewPresenter()
    {
    }
  }
}