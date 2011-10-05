#include "MantidQtCustomInterfaces/InelasticISIS.h"
#include "MantidQtCustomInterfaces/LatticePresenter.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    InelasticISIS::InelasticISIS(LoanedMemento& wsMemento) : m_WsMemento(wsMemento)
    {
    }

    ParameterisedLatticeView* InelasticISIS::createLatticeView() 
    {
      return new ParameterisedLatticeView(new LatticePresenter(m_WsMemento));
      //TODO More methods for this Abstract Factory
    }
  }
}