#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEVIEW_H_

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/IRemoteJobManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconFiltersSettings.h"
#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Tomography GUI. Base class / interface for the view of the tomo GUI
(view in the sense of the MVP pattern). This class is Qt-free. Qt
specific functionality/dependencies are added in a class derived from
this.

Copyright &copy; 2014-2016 ISIS Rutherford Appleton Laboratory, NScD
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
class ITomographyIfaceView {

public:
  ITomographyIfaceView(){};
  virtual ~ITomographyIfaceView(){};

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
   * Set the compute resources available (remote and/or local). Note
   * that the order of the list is/can be important.
   *
   * @param resources identifiers for the resources (their display names
   * normally). Order matters
   *
   * @param enabled tell if the resources are enabled or usable (which
   * should have an effect on how they are displayed and if they can be
   * used (for example, selected) in the interface.
   */
  virtual void setComputeResources(const std::vector<std::string> &resources,
                                   const std::vector<bool> &enabled) = 0;

  /**
   * Set the tools available on a compute resource.
   *
   * @param tools identifiers of the tools that can or could be run.
   * Order matters
   *
   * @param enabled tells if the tools are actually usable (which
   * should have an effect on how they are displayed and if they can
   * be used in the GUI (for example, if they can be selected at all).
   */
  virtual void setReconstructionTools(const std::vector<std::string> &tools,
                                      const std::vector<bool> &enabled) = 0;

  /**
   * Gives messages that this View wants to send to the logging
   * system.
   *
   * @return list of messages to log, one by one.
   */
  virtual std::vector<std::string> logMsgs() const = 0;

  /**
   * Reference or ID of the experiment entered/selected by the
   * user. The ID is effectively the RBNumber (defined at ISIS as the
   * "experiment reference number from the proposal"). The RBNumber
   * identifies one experiment or a set of experiments from an
   * approved experiment proposal. See for example:
   * www.isis.stfc.ac.uk/groups/computing/data/problems-finding-your-data11691.html
   *
   * @return identifier as a string
   */
  virtual std::string experimentReference() const = 0;

  /**
   * Username entered by the user
   *
   * @return username to log in to the compute resource
   */
  virtual std::string getUsername() const = 0;

  /**
   * Password entered by the user
   *
   * @return password to authenticate / log in to the compute resource
   */
  virtual std::string getPassword() const = 0;

  virtual std::vector<std::string> processingJobsIDs() const = 0;

  /**
   * Get the current system settings. This includes several
   * parameters. Most of them are paths or path components them, but
   * there are also some naming conventions and other parameters.
   *
   * @return Settings with current values (possibly modified by the
   * user).
   */
  virtual TomoSystemSettings systemSettings() const = 0;

  /**
   * Get the current reconstruction tool settings set by the
   * user. This is about tool specific options (like reconstruction
   * method, etc.).
   *
   * @return Settings for the set of supported tools.
   */
  virtual TomoReconToolsUserSettings reconToolsSettings() const = 0;

  /**
   * The filters settings defined by the user. These options are
   * general pre-/post-processing options.
   *
   * @return Pre-/post-processing filters settings
   */
  virtual TomoReconFiltersSettings prePostProcSettings() const = 0;

  /**
   * Gets the compute resource that is currently selected by the user.
   * This calls a validation method that can throw in case of
   * inconsistencies.
   *
   * @return Name of the compute resource as a string.
   */
  virtual std::string currentComputeResource() const = 0;

  /**
   * What's the reconstruction tool currently used and/or set up by
   * the user.
   *
   * @return name of the tool as a human readable string
   */
  virtual std::string currentReconTool() const = 0;

  /**
   * Method/algorithm selected from the TomoPy list.
   *
   * @return name of the method as used in TomoPy
   */
  virtual std::string astraMethod() const = 0;

  /**
   * Method/algorithm selected from the Astra list.
   *
   * @return name of the method as used in Astra Toolbox
   */
  virtual std::string tomopyMethod() const = 0;

  /**
   * Updates buttons and banners related to the current login
   * status. For example, when we are logged in, the 'log in' button
   * should be disabled, but some other widget may be enabled or some
   * way displaying the 'in' status.
   *
   * @param loggedIn if we're logged in.
   */
  virtual void updateLoginControls(bool loggedIn) = 0;

  /**
   * To enable/disable the actions that require being logged in. This
   * can include for example: submit jobs, get or refresh status of
   * jobs, setup parameters for the remote compute resource, logout,
   * etc.
   *
   * @param enable True to enable, or allow the user to use buttons
   * and widgets to trigger those actions.
   */
  virtual void enableLoggedActions(bool enable) = 0;

  /**
   * Whether to enable (allow the user to open or use) the tool
   * specific configuration dialog.
   *
   * @param on enable the button or mechanism to open the dialog
   */
  virtual void enableConfigTool(bool on) = 0;

  /**
   * Set the state of the 'reconstruct' button or similar to start
   * reconstruction jobs.
   *
   * @param on whether to enable / allow the user to run it
   */
  virtual void enableRunReconstruct(bool on) = 0;

  /**
   * Path to the image (2D) being shown on the interface.
   *
   * @return path as a string
   */
  virtual std::string showImagePath() const = 0;

  /**
   * Draw an image on the visualization tab/interface, from a path to
   * a recognized image format. Here recognized format means something
   * that is supported natively by the widgets library, in practice
   * Qt. Normally you can expect that .tiff and .png images are
   * supported.
   *
   * @param path path to the image file.
   */
  virtual void showImage(const std::string &path) = 0;

  /**
   * Draw an image on the visualization tab/interface, from a matrix
   * workspace (for example when elsewhere you load a FITS or similar
   * format that is loaded through a Mantid algorithm, say LoadFITS,
   * and you want to display it here). This should check dimensions
   * and workspace structure and shows user warning/error messages
   * appropriately. But in principle it should not raise any
   * exceptions under reasonable circumstances. It assumes that the
   * workspace contains an image in the form in which LoadFITS loads
   * FITS images (or spectrum per row, all of them with the same
   * number of data points (columns)).
   *
   * @param ws Workspace where a FITS or similar image has been loaded
   * with LoadFITS or similar algorithm.
   */
  virtual void showImage(const Mantid::API::MatrixWorkspace_sptr &ws) = 0;

  /**
   * Paths to the sample, open beam, etc. data (image) files.
   *
   * @return paths configuration object
   */
  virtual TomoPathsConfig currentPathsConfig() const = 0;

  /**
   * Takes paths produces programmatically and displays them to the
   * user. This can be used for example when modifying the paths based
   * on some logic/consistency checks outside of this view.
   *
   * @param cfg configuration to use from now on
   */
  virtual void updatePathsConfig(const TomoPathsConfig &cfg) = 0;

  /**
   * Regions and center of rotation, normally defined by the user with
   * a graphical rectangle selection tool.
   *
   * @return current user selection of regions
   */
  virtual ImageStackPreParams currentROIEtcParams() const = 0;

  /**
   * Show a tool specific configuration dialog for the user to set it up
   *
   * @param name human readable name of the tool, as a string
   */
  virtual void showToolConfig(const std::string &name) = 0;

  /**
   * Refresh the table, tree etc. that displays info on the running/finished
   *jobs.
   *
   * @param status Job information, as produced for example by the
   * Mantid remote algorithms.
   *
   * @param localStatus similar information but for local runs
   */
  virtual void updateJobsInfoDisplay(
      const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &status,
      const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &
          localStatus) = 0;

  /**
   * Save settings (normally when closing the interface). This refers
   * to purely GUI settings, such as window max/min status and  geometry,
   * preferences and niceties of the user interface.
   */
  virtual void saveSettings() const = 0;

  /**
   * If using the keep alive mechanism, what's the period for the
   * query to the remote compute resources? 0 == disabled (by default)
   *
   * @return period (in seconds) for the keep alive mechanism, when
   * using it; 0 otherwise.
   */
  virtual int keepAlivePeriod() { return 0; }

  /**
   * Get parameters for wavelength/energy bands aggregation. Provides
   * all the parameters needed to run the aggregation algorithm.
   *
   * @return parameters for an algorithm, as key-value strings
   * for every required parameter and optinal parameter set to
   * non-default values.
   */
  virtual std::map<std::string, std::string>
  currentAggregateBandsParams() const = 0;

  /**
   * Run the wavelength/energy bands aggregation algorithm in the
   * background.
   *
   * @param alg algorithm initialized and ready to run.
   */
  virtual void runAggregateBands(Mantid::API::IAlgorithm_sptr alg) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_ITOMOGRAPHYIFACEVIEW_H_
