#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_

#include "IEnggDiffMultiRunFittingWidgetOwner.h"

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetAdder {

public:
  virtual ~IEnggDiffMultiRunFittingWidgetAdder() = default;
  virtual void operator()(IEnggDiffMultiRunFittingWidgetOwner &owner) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_
