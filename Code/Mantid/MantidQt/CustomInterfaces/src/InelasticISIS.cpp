#include "MantidQtCustomInterfaces/InelasticISIS.h"
#include "MantidQtCustomInterfaces/LatticePresenter.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    Constructor
    @param wsMemento : ref to LoanedMemento
    */
    InelasticISIS::InelasticISIS(LoanedMemento& wsMemento) : m_WsMemento(wsMemento)
    {
    }

    /**
    Creational method for the lattice view.
    @return a new ParameterisedLatticeView
    */
    ParameterisedLatticeView* InelasticISIS::createLatticeView() 
    {
      return new ParameterisedLatticeView(new LatticePresenter(m_WsMemento));
    }

    /**
    Creational method for the log view.
    @return a new Standard log view
    */
    StandardLogView* InelasticISIS::createLogView()
    {
      return new StandardLogView(new LogPresenter(m_WsMemento));
    }
  }
}