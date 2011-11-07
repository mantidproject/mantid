#ifndef MANTIDQTCUSTOMINTERFACES_INELASTIC_ISIS_H_
#define MANTIDQTCUSTOMINTERFACES_INELASTIC_ISIS_H_

#include "MantidQtCustomInterfaces/Approach.h"
#include "MantidQtCustomInterfaces/ParameterisedLatticeView.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"
#include "MantidQtCustomInterfaces/StandardLogView.h"
#include "MantidQtCustomInterfaces/EditableLogView.h"


namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Group configuration for the ISIS Inelastic team.
    class DLLExport InelasticISIS : public Approach
    {
    public:
      InelasticISIS();
      virtual ParameterisedLatticeView* createLatticeView(LatticePresenter_sptr presenter);
      virtual StandardLogView* createLogView(LogPresenter_sptr presenter);
      virtual EditableLogView* createEditableLogView(LogPresenter_sptr presenter);
    };
  }
}

#endif