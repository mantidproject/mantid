#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingPresenter {

public:
  virtual ~IEnggDiffGSASFittingPresenter() = default;

  // User actions, triggered by the (passive) view,
  // which need handling in implementation
  enum Notification {
    DoRefinement, ///< Perform a GSAS refinement on a run
    LoadRun,      ///< Load a focused run
    SelectRun,    ///< The user has selected a different run in the multi-run
                  /// widget
    ShutDown,     ///< Shut down the interface
    Start,        ///< Start and setup the interface
  };

  /**
   * Notifications sent through the presenter when something changes
   * in the view. This plays the role of signals emitted by the view
   * to this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void notify(IEnggDiffGSASFittingPresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGPRESENTER_H_
