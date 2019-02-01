// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONUSERMSG_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONUSERMSG_H_

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface for user message related functionality in the engineering
diffraction custom interface / GUI view(s). This can be used in
different tabs/widgets as well as in the main/central view. Normally
the individual / area specific tabs/widgets will forward to the main
view.
*/
class IEnggDiffractionUserMsg {

public:
  virtual ~IEnggDiffractionUserMsg() = default;

  /**
   * Display the current status (running some algorithms, finished,
   * ready, etc.), in a status bar or similar.
   *
   * @param sts status message which should be concise
   */
  virtual void showStatus(const std::string &sts) = 0;

  /// @name Direct (and usually modal, or at least top/pop-up level) user
  /// interaction
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
  //@}

  /**
   * Enable/disable all user actions to
   * calibrate+focus+fitting+.... The idea is that actions / buttons
   * like 'calibrate', 'load calibration', 'focus', 'fit' can be
   * disabled while other work is done (be it calibration, focusing,
   * fitting or anything).
   *
   * @param enable true to enable actions (default initial state)
   */
  virtual void enableCalibrateFocusFitUserActions(bool enable) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONUSERMSG_H_
