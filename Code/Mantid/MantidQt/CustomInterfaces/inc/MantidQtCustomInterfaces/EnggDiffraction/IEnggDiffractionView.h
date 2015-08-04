#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffCalibSettings.h"
// #include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffSettings.h"

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
   * Gives messages that this View wants to send to the logging
   * system.
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
   * Save settings (normally when closing the interface). This
   * concerns only GUI settings, such as window max/min status and
   * geometry, preferences etc. of the user interface.
   */
  virtual void saveSettings() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEW_H_
