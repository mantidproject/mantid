// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGPRESENTER_H_

#include "IEnggDiffractionCalibration.h"
#include "IEnggDiffractionParam.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface of the presenter of the fitting widget/tab of the Engg
Diffraction GUI. Here the term presenter is used as in the MVP
(Model-View-Presenter) pattern. The view should be as passive as
possible and just notify any user actions.
*/
class IEnggDiffFittingPresenter {

public:
  /// These are user actions, triggered from the (passive) view, that need
  /// handling by the presenter
  enum Notification {
    Start,       ///< Start and setup interface
    Load,        ///< Load the focused file to the canvas
    FitPeaks,    ///< Preforms single peak fits
    FitAllPeaks, ///< Preforms multiple runs in sequence single peak fits
    addPeaks,    ///< Adds peak to the list
    browsePeaks, ///< Browse peaks to the list
    savePeaks,   ///< Save the peaks list
    ShutDown,    ///< closing the interface
    LogMsg,      ///< need to send a message to the Mantid log system
    selectRun,   ///< update plot with new run selected from list widget
    removeRun,   ///< remove a run from the model and the list widget
    updatePlotFittedPeaks ///< hide/show fitted peaks in the plot as required
  };

  /**
   * Notifications sent through the presenter when something changes
   * in the view. This plays the role of signals emitted by the view
   * to this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void notify(IEnggDiffFittingPresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGPRESENTER_H_
