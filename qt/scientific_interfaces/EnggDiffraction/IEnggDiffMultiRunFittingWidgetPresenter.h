#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_

#include "IEnggDiffMultiRunFittingWidgetAdder.h"
#include "RunLabel.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetPresenter {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetPresenter() = default;

  // User actions, triggered by the (passive) view,
  // which need handling in implementation
  enum class Notification {
    PlotPeaksStateChanged, ///< Change whether fitted peaks are plotted
    PlotToSeparateWindow,  ///< Plot currently selected run to a separate window
    RemoveRun,             ///< Remove a run from the list
    SelectRun,             ///< The user has selected a new run from the list
  };

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
   @param ws The workspace to add
  */
  virtual void addFocusedRun(const Mantid::API::MatrixWorkspace_sptr ws) = 0;

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

  /// Get RunLabels for all runs loaded into the tab
  virtual std::vector<RunLabel> getAllRunLabels() const = 0;

  /// Get run number and bank ID of the run currently selected in the list
  virtual boost::optional<RunLabel> getSelectedRunLabel() const = 0;

  /// Get functor to add this widget to a parent
  virtual std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
  getWidgetAdder() const = 0;

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
