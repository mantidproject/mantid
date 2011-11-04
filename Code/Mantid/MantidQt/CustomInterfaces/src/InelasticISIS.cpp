#include "MantidQtCustomInterfaces/InelasticISIS.h"
#include "MantidQtCustomInterfaces/LatticePresenter.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    Constructor
    */
    InelasticISIS::InelasticISIS()
    {
    }

    /**
    Creational method for the lattice view.
    @return a new ParameterisedLatticeView
    */
    ParameterisedLatticeView* InelasticISIS::createLatticeView(boost::shared_ptr<LatticePresenter> presenter)
    {
      return new ParameterisedLatticeView(presenter);
    }

    /**
    Creational method for the log view.
    @return a new Standard log view
    */
    StandardLogView* InelasticISIS::createLogView(boost::shared_ptr<LogPresenter> presenter)
    {
      return new StandardLogView(presenter);
    }
  }
}