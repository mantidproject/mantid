#ifndef MANTIDQTCUSTOMINTERFACES_INELASTIC_ISIS_H_
#define MANTIDQTCUSTOMINTERFACES_INELASTIC_ISIS_H_

#include "MantidQtCustomInterfaces/Approach.h"
#include "MantidQtCustomInterfaces/ParameterisedLatticeView.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"
#include "MantidQtCustomInterfaces/StandardLogView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Group configuration for the ISIS Inelastic team.
    class DLLExport InelasticISIS : public Approach
    {
    public:
      InelasticISIS(LoanedMemento& memento);
      virtual ParameterisedLatticeView* createLatticeView();
      virtual StandardLogView* createLogView();
    private:
      /// Memento containing all the meta data of interest for the workspace.
      LoanedMemento m_WsMemento;
    };
  }
}

#endif