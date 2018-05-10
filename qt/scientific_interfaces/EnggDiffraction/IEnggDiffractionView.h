#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_

#include <string>
#include <vector>

#include "IEnggDiffractionPythonRunner.h"
#include "IEnggDiffractionSettings.h"
#include "IEnggDiffractionUserMsg.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Engineering diffraction custom interface / GUI. This is the base class
(interface) for the view of the engineering diffraction GUI ((view in
the sense of the Model-View-Presenter, MVP pattern). This class is
Qt-free. Qt specific functionality/dependencies are added in a class
derived from this.

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
class IEnggDiffractionView : public IEnggDiffractionUserMsg,
                             public IEnggDiffractionSettings,
                             public IEnggDiffractionPythonRunner {

public:
  virtual ~IEnggDiffractionView() = default;

  /// @name Direct (and usually modal, or at least top/pop-up level) user
  /// interaction
  //@{
  /**
   * To display important messages that need maximum visibility
   * (normally a dialog on top of the interface). This can be used to
   * control the visibility and content of the message. An example use
   * case is to inform the user that certain inputs are absolutely
   * needed to use the interface functionality.
   *
   * @param visible whether the "splash"/important message should be visible
   * @param shortMsg short/one line message summary
   * @param description message with full details
   */
  virtual void splashMessage(bool visible, const std::string &shortMsg,
                             const std::string &description) = 0;

  /**
   * Gets a filename from the user, to use for a new calibration file.
   *
   * @param suggestedFname filename that the user can just accept without
   * typing anything
   *
   * @return filename (can be empty if not given or an error happened)
   */
  virtual std::string
  askNewCalibrationFilename(const std::string &suggestedFname) = 0;

  /**
   * Gets an (existing file) filename from the user, to load a calibration.
   *
   * @return filename given by the user (empty if cancelled / not interested)
   */
  virtual std::string askExistingCalibFilename() = 0;
  //@}

  /**
   * Gives one or more messages that this View wants to send to the
   * logging system.
   *
   * @return list of messages to log, one by one.
   */
  virtual std::vector<std::string> logMsgs() const = 0;

  /**
   * RB Number entered by the user
   *
   * @return RB number as string as provided by the user
   */
  virtual std::string getRBNumber() const = 0;

  /**
   * What's the instrument this interface is using?
   *
   * @return current instrument selection
   */
  virtual std::string currentInstrument() const = 0;

  /**
   * selected spec will be passed as a bank for the calibrartion
   * process to be carried out
   *
   * @return Bank selection index: spectrum-numbers / north / south
   */
  virtual int currentCropCalibBankName() const = 0;

  /**
   * customised spec will be passed via specNo text field for the
   * cropped calibrartion process to be carried out
   *
   * @return which format should to applied for plotting data
   */
  virtual std::string currentCalibSpecNos() const = 0;

  /**
   * customised bank name will be passed with SpectrumNos to
   * save workspace and file with particular bank name
   *
   * @return string which will be used to generate bank name
   */
  virtual std::string currentCalibCustomisedBankName() const = 0;

  /**
   * Selected plot data representation will be applied, which will
   * ran through python script
   *
   * @return which format should to applied for plotting data
   */
  virtual int currentPlotType() const = 0;

  /**
   * Selected multi-run focus mode
   *
   * @return return integer to the presenter
   */
  virtual int currentMultiRunMode() const = 0;

  /**
   * The Vanadium run number used in the current calibration
   *
   * @return Vanadium run number, as a string
   */
  virtual std::string currentVanadiumNo() const = 0;

  /**
   * The Ceria (CeO2) run number used in the current calibration
   *
   * @return Ceria run number, as a string
   */
  virtual std::string currentCeriaNo() const = 0;

  /**
   * The filename of the current calibration (corresponding to the
   * current Vanadium, Ceria)
   *
   * @return filename (normally full path)
   */
  virtual std::string currentCalibFile() const = 0;

  /**
   * The Vanadium run number to use for a new calibration
   *
   * @return Vanadium run number, as a string
   */
  virtual std::vector<std::string> newVanadiumNo() const = 0;

  /**
   * The Ceria (CeO2) run number to use for a new calibration
   *
   * @return Ceria run number, as a string
   */
  virtual std::vector<std::string> newCeriaNo() const = 0;

  /**
   * A new calibration is calculated or loaded => update display and
   * widgets. This becomes the new 'current' calibration.
   *
   * @param vanadiumNo new Vanadium run number
   * @param ceriaNo new Ceria run number
   * @param fname new calibration file name
   */
  virtual void newCalibLoaded(const std::string &vanadiumNo,
                              const std::string &ceriaNo,
                              const std::string &fname) = 0;

  /**
   * Enable/disable all the sections or tabs of the interface. To be
   * used with required parameters, like a valid instrument, a valid
   * RB number, etc. This should effectively disable/enable all
   * actions, including calibration, focusing, event mode, etc.
   *
   * @param enable true to enable all tabs of the interface
   */
  virtual void enableTabs(bool enable) = 0;

  /**
   * A (sample) run to focus
   *
   * @return run number, as a string
   */
  virtual std::vector<std::string> focusingRunNo() const = 0;

  /**
   * A (sample) run to focus, in "cropped" mode
   *
   * @return run number, as a string
   */
  virtual std::vector<std::string> focusingCroppedRunNo() const = 0;

  /**
   * A (sample) run to focus, in "texture" mode
   *
   * @return run number, as a string
   */
  virtual std::vector<std::string> focusingTextureRunNo() const = 0;

  /**
   * Banks to consider when focusing
   *
   * @return vector with a boolean value that tells if the
   * corresponding instrument bank numbers should be focused
   */
  virtual std::vector<bool> focusingBanks() const = 0;

  /**
   * Specification of spectrum Nos for focus in "cropped" mode.
   *
   * @return spectrum Nos, expected as a comma separated list of
   * integers or ranges of integers.
   */
  virtual std::string focusingCroppedSpectrumNos() const = 0;

  /**
   * Detector grouping file, used when focusing in "texture" mode.
   *
   * @return name of the grouping file with texture bank definitions
   */
  virtual std::string focusingTextureGroupingFile() const = 0;

  /**
   * Check box to consider when focusing
   * whether to plot focused workspace
   *
   * @return bool
   */
  virtual bool focusedOutWorkspace() const = 0;

  /**
   * Check box to consider when calibrating
   * whether to plot focused workspace
   *
   * @return bool
   */
  virtual bool plotCalibWorkspace() const = 0;

  /**
   * Reset all focus inputs/options
   */
  virtual void resetFocus() = 0;

  /// @name Pre-processing (of event data, rebinning)
  //@{
  /**
   * One or more run numbers to pre-process.
   *
   * @return run number(s), as a string
   */
  virtual std::vector<std::string> currentPreprocRunNo() const = 0;

  /**
   * For when pre-processing from event to histo data using a regular
   * time bin. Here time refers to time units for rebinning in
   * time-of-flight.
   *
   * @return time bin to re-bin in microseconds
   */
  virtual double rebinningTimeBin() const = 0;

  /**
   * For when pre-processing from multiperiod event to histo data.
   *
   * @return number of periods to use
   */
  virtual size_t rebinningPulsesNumberPeriods() const = 0;

  /**
   * For when pre-processing from multiperiod event to histo data.
   *
   * @return the time parameter (bin width) when rebinning by pulses.
   */
  virtual double rebinningPulsesTime() const = 0;

  //@}

  /**
   * Save settings (normally when closing the interface). This
   * concerns only GUI settings, such as window max/min status and
   * geometry, preferences etc. of the user interface.
   */
  virtual void saveSettings() const = 0;

  /**
   * Saves the ouput files which are generated, this can be done
   * via Output Files checkbox on the focus tab
   *
   * @return bool
   */
  virtual bool saveFocusedOutputFiles() const = 0;

  /**
   * Show the message requesting the user to enter a valid RB number, if the
   * current RB number is not valid
   * @param rbNumberIsValid True if the current entered RB number is not valid
   */
  virtual void showInvalidRBNumber(const bool rbNumberIsValid) = 0;

  /**
   * Produces vanadium curves graph with three spectrum and
   * ceria peaks graph with two spectrum for calib output.
   *
   * @param pyCode string which is passed to Mantid via pyScript
   */
  virtual void plotCalibOutput(const std::string &pyCode) = 0;

  /**
   * Produces a single spectrum graph for focused output.
   *
   * @param wsName name of the workspace to plot (must be in the ADS)
   */
  virtual void plotFocusedSpectrum(const std::string &wsName) = 0;

  /**
   * Produces a waterfall spectrum graph for focused output.
   *
   * @param wsName name of the workspace to plot (must be in the ADS)
   */
  virtual void plotWaterfallSpectrum(const std::string &wsName) = 0;

  /**
   * Produces a replaceable spectrum graph for focused output.
   *
   * @param wsName name of the workspace to plot (must be in the ADS)
   * @param spectrum number of the workspace to plot
   * @param type of the workspace plot
   */
  virtual void plotReplacingWindow(const std::string &wsName,
                                   const std::string &spectrum,
                                   const std::string &type) = 0;

  /**
   * Updates the instrument in all child tabs
   *
   * @param newInstrument name of the new instrument that will be set
   */
  virtual void updateTabsInstrument(const std::string &newInstrument) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_
