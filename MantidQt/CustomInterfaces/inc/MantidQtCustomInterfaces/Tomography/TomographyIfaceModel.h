#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEMODEL_H_

#include "MantidAPI/IRemoteJobManager.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"
#include "MantidQtCustomInterfaces/Tomography/TomoRecToolConfig.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconFiltersSettings.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"
#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettings.h"

// Include instead of forward declare so we have definition of qint64
#include <QMutex>

namespace MantidQt {
namespace CustomInterfaces {

class TomographyProcess;
class TomographyThread;
/**
Tomography GUI. Model for the interface (as in the MVP
(Model-View-Presenter) pattern). In principle, in a strict MVP setup,
signals from this model should always be handled through the presenter
and never go directly to the view.

Copyright &copy; 2014,2016 ISIS Rutherford Appleton Laboratory, NScD
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
class MANTIDQT_CUSTOMINTERFACES_DLL TomographyIfaceModel {
  // For now, there is no need to  make this a Q_OBJECT (and derive from
  // QObject, but this class may become a "full" Q_OBJECT as soon as a Qt
  // model (table, tree, etc.), with its signals, is used here
  // Q_OBJECT

public:
  /// Default constructor
  TomographyIfaceModel();
  virtual ~TomographyIfaceModel();

  void setupComputeResource();
  void setupRunTool(const std::string &compRes);

  /// is the current facility supported? (only ISIS for now)
  bool facilitySupported();

  /// the compure resources supported and their status/availability
  std::vector<std::string> computeResources() { return m_computeRes; }
  std::vector<bool> computeResourcesStatus() { return m_computeResStatus; }

  /// the reconstruction tools supported and their status/availability
  std::vector<std::string> reconTools() { return m_reconTools; }
  std::vector<bool> reconToolsStatus() { return m_reconToolsStatus; }

  // Mantid::Kernel::Logger & logger() const;

  const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
  jobsStatus() const {
    return m_jobsStatus;
  }

  const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo>
  jobsStatusLocal() const {
    return m_jobsStatusLocal;
  }

  /// Username last logged in, if any.
  std::string loggedIn() const { return m_loggedInUser; }

  // TODO: add companion currentComputeResource where LoggedIn() is in
  //--------------------------------------------
  // Current tool Settings
  //--------------------------------------------
  void usingTool(const std::string &tool) { m_currentToolName = tool; }
  std::string usingTool() const { return m_currentToolName; }

  void setCurrentToolMethod(std::string toolMethod) {
    m_currentToolMethod = toolMethod;
  }
  std::string getCurrentToolMethod() const { return m_currentToolMethod; }

  void setCurrentToolSettings(std::shared_ptr<TomoRecToolConfig> settings) {
    m_currentToolSettings = settings;
  }

  //--------------------------------------------
  // Access to the system settings information
  //--------------------------------------------
  // get the remote scripts base dir from the system settings
  std::string getCurrentRemoteScriptsBasePath() const {
    return m_systemSettings.m_remote.m_basePathReconScripts;
  }

  /// get the local paths from the system settings
  std::string getCurrentLocalScriptsBasePath() const {
    return m_systemSettings.m_local.m_reconScriptsPath;
  }

  std::string getExeternalInterpreterPath() const {
    return m_systemSettings.m_local.m_externalInterpreterPath;
  }

  /// get the experiment reference currently selected
  std::string getCurrentExperimentReference() const {
    return m_systemSettings.m_experimentReference;
  }

  /// returns the scripts folder, used for remote path
  std::string getTomoScriptFolderPath() const { return g_tomoScriptFolderPath; }

  /// returns the tomo script location path inside the scripts folder
  std::string getTomoScriptLocationPath() const {
    return g_mainReconstructionScript;
  }

  /// ping the (remote) compute resource
  bool doPing(const std::string &compRes);
  /// Log into the (remote or local) compute resource
  void doLogin(const std::string &compRes, const std::string &user,
               const std::string &pw);
  /// Log out from the (remote or local) compute resource
  void doLogout(const std::string &compRes, const std::string &username);
  /// Query the status of running (and recent) jobs
  void doQueryJobStatus(const std::string &compRes,
                        std::vector<std::string> &ids,
                        std::vector<std::string> &names,
                        std::vector<std::string> &status,
                        std::vector<std::string> &cmds);
  /// Submit a new job to the (remote or local) compute resource
  void prepareSubmissionArguments(const bool local, std::string &runnable,
                                  std::vector<std::string> &args,
                                  std::string &allOpts);

  void doLocalRunReconstructionJob(
      const std::string &runnable, const std::vector<std::string> &args,
      const std::string &allOpts,
      MantidQt::CustomInterfaces::TomographyThread &thread,
      MantidQt::CustomInterfaces::TomographyProcess &worker);

  void doRemoteRunReconstructionJob(const std::string &compRes,
                                    const std::string &runnable,
                                    const std::string &allOpts);

  /// Cancel a previously submitted job
  void doCancelJobs(const std::string &compRes,
                    const std::vector<std::string> &id);

  void addJobToStatus(const qint64 pid, const std::string &runnable,
                      const std::string &allOpts);
  /// Get fresh status information on running/recent jobs
  void doRefreshJobsInfo(const std::string &compRes);

  void refreshLocalJobsInfo();

  void setExperimentReference(const std::string ref) { m_experimentRef = ref; }

  /// Update to the current setings given by the user
  void setSystemSettings(const TomoSystemSettings &settings) {
    m_systemSettings = settings;
  }

  void setPrePostProcSettings(const TomoReconFiltersSettings &filters) {
    m_prePostProcSettings = filters;
  }

  void setImageStackPreParams(const ImageStackPreParams &roiEtc) {
    m_imageStackPreParams = roiEtc;
  }

  /// loads an image/picture in FITS format
  Mantid::API::WorkspaceGroup_sptr loadFITSImage(const std::string &path);

  /// Log this message through the system logging
  void logMsg(const std::string &msg);
  void logErrMsg(const std::string &msg);

  /// for clean destruction
  void cleanup();

  std::string localComputeResource() const { return g_LocalResourceName; }

  void setTomoPathsConfig(const TomoPathsConfig &tc) { m_pathsConfig = tc; }
  void updateProcessInJobList(const qint64 pid, const int exitCode);
  void terminateProcess();

  // Names of reconstruction tools
  static const std::string g_TomoPyTool;
  static const std::string g_AstraTool;
  static const std::string g_customCmdTool;
  // not supported yet
  static const std::string g_CCPiTool;
  static const std::string g_SavuTool;

  // Name of the remote compute resource
  static const std::string g_SCARFName;
  static const std::string g_LocalResourceName;

  // The main tomo_reconstruct.py or similar script (as it is distributed with
  // Mantid). This is the entry point for reconstruction jobs.
  static const std::string g_mainReconstructionScript;
  static const std::string g_tomoScriptFolderPath;

protected: // protected to expose everything to testing
  std::string
  constructSingleStringFromVector(const std::vector<std::string> args) const;

  /// retrieve info from compute resource into status table
  void getJobStatusInfo(const std::string &compRes);

  std::string validateCompResource(const std::string &res) const;

  bool checkIfToolIsSetupProperly(const std::string &tool,
                                  const std::string &cmd,
                                  const std::vector<std::string> &args) const;

  void makeTomoRecScriptOptions(const bool local,
                                std::vector<std::string> &opts) const;

  void filtersCfgToCmdOpts(const TomoReconFiltersSettings &filters,
                           const ImageStackPreParams &corRegions,
                           const bool local,
                           std::vector<std::string> &opts) const;

  void splitCmdLine(const std::string &cmd, std::string &runnable,
                    std::string &opts) const;

  /// process the tool name to be appropriate for the command line arg
  std::string prepareToolNameForArgs(const std::string &toolName) const;

  void checkDataPathsSet() const;

  std::string adaptInputPathForExecution(const std::string &path,
                                         const bool local) const;

  std::string buildOutReconstructionDir(const std::string &samplesDir,
                                        bool) const;

  bool processIsRunning(qint64 pid) const;

  std::string
  buildOutReconstructionDirFromSystemRoot(const std::string &samplesDir,
                                          bool local) const;

  std::string
  buildOutReconstructionDirFromSamplesDir(const std::string &samplesDir) const;

private:
  /// facility for the remote compute resource
  const std::string m_facility;

  /// Experiment reference (RBNumber for example)
  std::string m_experimentRef;

  /// username last logged in (empty if not in login status), from local
  /// perspective
  std::string m_loggedInUser;
  std::string m_loggedInComp;

  /// compute resources suppoted by this GUI (remote ones, clusters, etc.)
  std::vector<std::string> m_computeRes;
  /// status of the compute resources: availalbe/unavailable or usable/unusable
  std::vector<bool> m_computeResStatus;
  /// identifiers of reconstruction tools supported, given a compute resource
  std::vector<std::string> m_reconTools;
  std::vector<bool> m_reconToolsStatus;

  // status of remote jobs
  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> m_jobsStatus;
  // status of local runs/jobs
  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> m_jobsStatusLocal;

  /// reconstruction tools available on SCARF
  std::vector<std::string> m_SCARFtools;

  // Paths for the sample, flat and dark images
  TomoPathsConfig m_pathsConfig;

  // System settting including several paths and parameters (local and remote)
  TomoSystemSettings m_systemSettings;

  //--------------------------------
  // Current tool variables
  //--------------------------------

  // current tool's name, updated from the presenter on change
  std::string m_currentToolName;

  // the tool settings so we can use it for reconstruction params
  std::shared_ptr<TomoRecToolConfig> m_currentToolSettings;

  // current tool's method, updated from the presenter on change
  std::string m_currentToolMethod;

  // Settings for the pre-/post-processing filters
  TomoReconFiltersSettings m_prePostProcSettings;

  // Parameters set for the ROI, normalization region, etc.
  ImageStackPreParams m_imageStackPreParams;

  // mutex for the job status info update operations
  // TODO: replace with std::mutex+std::lock_guard
  QMutex *m_statusMutex;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEMODEL_H_
