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
      InelasticISIS();
      virtual ParameterisedLatticeView* createLatticeView(boost::shared_ptr<LatticePresenter> presenter);
      virtual StandardLogView* createLogView(boost::shared_ptr<LogPresenter> presenter);
    };
  }
}

#endif