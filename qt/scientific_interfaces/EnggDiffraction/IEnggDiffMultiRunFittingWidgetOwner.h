#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETOWNER_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETOWNER_H_

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetView;

/// Interface for a view which contains a multi-run fitting widget
class IEnggDiffMultiRunFittingWidgetOwner {
public:
  /// Add a multi-run widget to the owner
  virtual void addWidget(IEnggDiffMultiRunFittingWidgetView *widget) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETOWNER_H_
