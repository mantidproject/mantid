#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_

#include "RunLabel.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetPresenter {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetPresenter() = default;

  /**
   Add a fitted peaks workspace to the widget, so it can be overplotted on its
   focused run
   @param runLabel Identifier of the workspace to add
   @param ws The workspace to add
  */
  virtual void addFittedPeaks(const RunLabel &runLabel,
                              const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /**
   Add a focused run to the widget. The run should be added to the list and
   plotting it should be possible
   @param runLabel Identifier of the workspace to add
   @param ws The workspace to add
  */
  virtual void addFocusedRun(const RunLabel &runLabel,
                             const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /**
   Get fitted peaks workspace corresponding to a given run and bank, if a fit
   has been done on that run
   @param runLabel Identifier of the workspace to get
   @return The workspace, or an empty optional if a fit has not been run
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const RunLabel &runLabel) const = 0;

  /**
   Get focused workspace corresponding to a given run and bank, if that run has
   been loaded into the widget
   @param runLabel Identifier of the workspace to get
   @return The workspace, or empty optional if that run has not been loaded in
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const RunLabel &runLabel) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
