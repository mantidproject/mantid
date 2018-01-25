#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetPresenter {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetPresenter() = default;

  // User actions, triggered by the (passive) view,
  // which need handling in implementation
  enum Notification {
    AddFittedPeaks, /// The view has been passed a fitted peaks workspace to add
    AddFocusedRun,  ///< The view has been passed a focused run to add
    ShutDown,       ///< Shut down the widget
    Start,          ///< Start and set up the interface
  };

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
