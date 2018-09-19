#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetAdder.h"

namespace MantidQt {
namespace CustomInterfaces {

class FakeEnggDiffMultiRunFittingWidgetAdder
    : public IEnggDiffMultiRunFittingWidgetAdder {
public:
  void operator()(IEnggDiffMultiRunFittingWidgetOwner &owner) override;
};

void FakeEnggDiffMultiRunFittingWidgetAdder::
operator()(IEnggDiffMultiRunFittingWidgetOwner &owner) {
  UNUSED_ARG(owner);
}

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETADDER_H_
