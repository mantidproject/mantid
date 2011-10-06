#ifndef MANTIDQTCUSTOMINTERFACES_INELASTIC_ISIS_H_
#define MANTIDQTCUSTOMINTERFACES_INELASTIC_ISIS_H_

#include "MantidQtCustomInterfaces/Approach.h"
#include "MantidQtCustomInterfaces/ParameterisedLatticeView.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //class ParameterisedLatticeView;
    class DLLExport InelasticISIS : public Approach
    {
    public:
      InelasticISIS(LoanedMemento& memento);
      virtual ParameterisedLatticeView* createLatticeView();
    private:
      LoanedMemento m_WsMemento;
    };
  }
}

#endif