#ifndef MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEVIEWMOCK_H
#define MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEVIEWMOCK_H

#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"

#include <gmock/gmock.h>

// This is a simple mock for the tomo interface view when using SCARF.
class MockTomographyIfaceView
    : public MantidQt::CustomInterfaces::ITomographyIfaceView {
public:
  // void userWarning(const std::string &warn, const std::string &description)
  // {}
  MOCK_METHOD2(userWarning,
               void(const std::string &warn, const std::string &description));

  // void userError(const std::string &err, const std::string &description) {}
  MOCK_METHOD2(userError,
               void(const std::string &err, const std::string &description));

  // std::vector<std::string> logMsgs() const {}
  MOCK_CONST_METHOD0(logMsgs, std::vector<std::string>());

  // void setComputeResources(const std::vector<std::string> &resources,
  //                         const std::vector<bool> &enabled) {}
  MOCK_METHOD2(setComputeResources,
               void(const std::vector<std::string> &resources,
                    const std::vector<bool> &enabled));

  // void setReconstructionTools(const std::vector<std::string> &tools,
  //                            const std::vector<bool> &enabled) {}
  MOCK_METHOD2(setReconstructionTools,
               void(const std::vector<std::string> &tools,
                    const std::vector<bool> &enabled));

  // void saveSettings() const {}
  MOCK_CONST_METHOD0(saveSettings, void());

  // std::string getPassword() const {}
  MOCK_CONST_METHOD0(getUsername, std::string());

  // std::string getPassword() const {}
  MOCK_CONST_METHOD0(getPassword, std::string());

  // std::vector<std::string> processingJobsIDs() const {}
  MOCK_CONST_METHOD0(processingJobsIDs, std::vector<std::string>());

  // std::string currentComputeResource() const {}
  MOCK_CONST_METHOD0(currentComputeResource, std::string());

  // std::string currentReconTool() const {}
  MOCK_CONST_METHOD0(currentReconTool, std::string());

  // void updateLoginControls(bool loggedIn) {}
  MOCK_METHOD1(updateLoginControls, void(bool loggedIn));

  // void enableLoggedActions(bool enable) {}
  MOCK_METHOD1(enableLoggedActions, void(bool enable));

  // void enableConfigTool(bool on) {}
  MOCK_METHOD1(enableConfigTool, void(bool on));

  // void enableRunReconstruct(bool on) {}
  MOCK_METHOD1(enableRunReconstruct, void(bool on));

  // std::string showImagePath() {}
  MOCK_CONST_METHOD0(showImagePath, std::string());

  // void showImage(const Mantid::API::MatrixWorkspace_sptr &wsg) {}
  MOCK_METHOD1(showImage, void(const Mantid::API::MatrixWorkspace_sptr &wsg));

  // void showImage(const std::string &path) {}
  MOCK_METHOD1(showImage, void(const std::string &path));

  // TomoPathsConfig currentPathsConfig() const {}
  MOCK_CONST_METHOD0(currentPathsConfig, MantidQt::CustomInterfaces::TomoPathsConfig());

  // void showToolConfig(const std::string &name) {}
  MOCK_METHOD1(showToolConfig, void(const std::string &name));

  // void updateJobsInfoDisplay(const std::vector<
  //    Mantid::API::IRemoteJobManager::RemoteJobInfo> &status) {}
  MOCK_METHOD1(updateJobsInfoDisplay,
               void(const std::vector<
                   Mantid::API::IRemoteJobManager::RemoteJobInfo> &status));

  // MantidQt::CustomInterfaces::TomoReconToolsUserSettings
  // reconToolsSettings() const {}
  MOCK_CONST_METHOD0(reconToolsSettings,
                     MantidQt::CustomInterfaces::TomoReconToolsUserSettings());
};

#endif // MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEVIEWMOCK_H
