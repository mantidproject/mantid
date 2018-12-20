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

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
