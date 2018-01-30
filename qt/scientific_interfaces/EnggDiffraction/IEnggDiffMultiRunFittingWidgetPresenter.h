#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetPresenter {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetPresenter() = default;

  // User actions, triggered by the (passive) view,
  // which need handling in implementation
  enum Notification {
    ShutDown,       ///< Shut down the widget
    Start,          ///< Start and set up the interface
  };

  /**
   Add a fitted peaks workspace to the widget, so it can be overplotted on its
   focused run
   @param runNumber The run number of the workspace to add
   @param bank The bank ID of the workspace to add
   @param ws The workspace to add
  */
  virtual void addFittedPeaks(const int runNumber, const size_t bank,
                              const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /**
   Add a focused run to the widget. The run should be added to the list and
   plotting it should be possible
   @param runNumber The run number of the workspace to add
   @param bank The bank ID of the workspace to add
   @param ws The workspace to add
  */
  virtual void addFocusedRun(const int runNumber, const size_t bank,
                             const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /**
   Get fitted peaks workspace corresponding to a given run and bank, if a fit
   has been done on that run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The workspace, or an empty optional if a fit has not been run
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  /**
   Get focused workspace corresponding to a given run and bank, if that run has
   been loaded into the widget
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The workspace, or empty optional if that run has not been loaded in
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const int runNumber, const size_t bank) const = 0;

  /**
   * Notifications sent through the presenter when something changes
   * in the view. This plays the role of signals emitted by the view
   * to this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void
  notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
