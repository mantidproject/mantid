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
    ParameterisedLatticeView* InelasticISIS::createLatticeView(LatticePresenter_sptr presenter)
    {
      return new ParameterisedLatticeView(presenter);
    }

    /**
    Creational method for the log view.
    @return a new Standard log view
    */
    StandardLogView* InelasticISIS::createLogView(LogPresenter_sptr presenter)
    {
      return new StandardLogView(presenter);
    }

    EditableLogView* InelasticISIS::createEditableLogView(LogPresenter_sptr presenter)
    {
      return new EditableLogView(presenter);
    }
  }
}