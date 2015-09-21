#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_

#include <string>
#include <vector>

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffCalibSettings.h"

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
class IEnggDiffractionView {

public:
  IEnggDiffractionView(){};
  virtual ~IEnggDiffractionView(){};

  /// @name Direct (and usually modal) user interaction
  //@{
  /**
   * Display a warning to the user (for example as a pop-up window).
   *
   * @param warn warning title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form description of the issue.
   */
  virtual void userWarning(const std::string &warn,
                           const std::string &description) = 0;

  /**
   * Display an error message (for example as a pop-up window).
   *
   * @param err Error title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form description of the issue.
   */
  virtual void userError(const std::string &err,
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
   * Username entered by the user
   *
   * @return username to log in to the compute resource
   */
  virtual std::string getRBNumber() const = 0;

  /**
   *
   * @return calibration settings object with current user settings
   */
  virtual EnggDiffCalibSettings currentCalibSettings() const = 0;

  /**
   * What's the instrument this interface is using?
   *
   * @return current instrument selection
   */
  virtual std::string currentInstrument() const = 0;

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
  virtual std::string newVanadiumNo() const = 0;

  /**
   * The Ceria (CeO2) run number to use for a new calibration
   *
   * @return Ceria run number, as a string
   */
  virtual std::string newCeriaNo() const = 0;

  /**
   * The filename (can be full path) selected to write a calibration
   *
   * @return file name
   */
  virtual std::string outCalibFilename() const = 0;

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
   * Write a GSAS file. Temporarily here until we have a more final
   * way of generating these files.
   *
   * @param outFilename output file name
   * @param difc difc values (one per bank)
   * @param tzero tzero values (one per bank)
   */
  virtual void writeOutCalibFile(const std::string &outFilename,
                                 const std::vector<double> &difc,
                                 const std::vector<double> &tzero) = 0;

  /**
   * Enable/disable calibrate+focus actions. The idea is that actions
   * / buttons like 'calibrate', 'load calibration', or 'focus' can be
   * disabled while a calibration of a focusing is being calculated.
   *
   * @param enable true to enable actions (default initial state)
   */
  virtual void enableCalibrateAndFocusActions(bool enable) = 0;

  /**
   * Directory set for focusing outputs
   *
   * @return directory path as a string
   */
  virtual std::string focusingDir() const = 0;

  /**
   * A (sample) run to focus
   *
   * @return run number, as a string
   */
  virtual std::string focusingRunNo() const = 0;

  /**
   * Bank to consider when focusing
   *
   * @return instrument bank number
   */
  virtual int focusingBank() const = 0;

  /**
   * Save settings (normally when closing the interface). This
   * concerns only GUI settings, such as window max/min status and
   * geometry, preferences etc. of the user interface.
   */
  virtual void saveSettings() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_
