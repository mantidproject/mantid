#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface for what the presenter of the Engg Diffraction GUI needs to
implement. Here the term presenter is used as in the MVP
(Model-View-Presenter) pattern. The view has been designed in
principle as a passive one that notifies any user action.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
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
class IEnggDiffractionPresenter {
public:
  virtual ~IEnggDiffractionPresenter() {}

  /// These are user actions, triggered from the (passive) view, that need
  /// handling by the presenter
  enum Notification {
    Start,             ///< Start and setup interface
    LoadExistingCalib, ///< Load a calibration already availble on disk
    CalcCalib,         ///< Calculate a (new) calibration
    CropCalib,         ///< Calculate a cropped calibration
    FocusRun,          ///< Focus one or more run files
    FocusCropped,      ///< Focus one or more run files, cropped variant
    FocusTexture,      ///< Focus one or more run files, texture variant
    ResetFocus,        ///< Re-set / clear all focus inputs and options
    RebinTime,         ///< From event to histo, with a time bin
    RebinMultiperiod,  ///< From event to histo, multiperiod event data
    FitPeaks,          ///< Preforms single peak fits
    LogMsg,            ///< need to send a message to the Mantid log system
    InstrumentChange,  ///< Instrument selection updated
    RBNumberChange,    ///< RBNumber filled-in/changed
    ShutDown,          ///< closing the interface
    StopFocus          ///< stopping the focus process
  };

  /**
   * Notifications sent through the presenter when something changes
   * in the view. This plays the role of signals emitted by the view
   * to this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void notify(IEnggDiffractionPresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPRESENTER_H_
