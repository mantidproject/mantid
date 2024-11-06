// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IALCDataLoadingModel.h"
#include "IALCDataLoadingView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include <atomic>

namespace MantidQt::CustomInterfaces {

class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingModel : public IALCDataLoadingModel {

public:
  ALCDataLoadingModel();
  ~ALCDataLoadingModel() override = default;
  void load(const std::string &log, const std::string &function, const std::string &calculationType,
            const std::string &deadTimeType, const std::string &deadTimeFile, const std::string &redPeriod,
            const std::optional<std::pair<double, double>> &timeRange, const std::string &detectorGroupingType,
            const std::string &forwardGrouping, const std::string &backwardGrouping, const std::string &alphaValue,
            const bool &subtractIsChecked, const std::string &greenPeriod) override;
  void cancelLoading() const override;
  Mantid::API::MatrixWorkspace_sptr exportWorkspace() override;
  bool checkCustomGrouping(const std::string &detGroupingType, const std::string &forwardGrouping,
                           const std::string &backwardGrouping) override;
  void updateAutoLoadCancelled() override;
  bool loadFilesFromWatchingDirectory(const std::string &firstFile, const std::vector<std::string> &files,
                                      const std::string &runsText) override;
  std::string getPathFromFiles(const std::vector<std::string> &files) const override;

  // Getters
  bool getLoadingData() const override;
  Mantid::API::MatrixWorkspace_sptr getLoadedData() const override;
  std::vector<std::string> &getLogs() override;
  std::vector<std::string> &getPeriods() override;
  Mantid::API::MatrixWorkspace_sptr getWsForMuonInfo() const override;
  double getMinTime() const override;
  std::string &getRunsText() override;

  // Setters
  void setLoadingData(bool isLoading) override;
  void setLoadedData(const Mantid::API::MatrixWorkspace_sptr &data) override;
  void setLogs(const Mantid::API::MatrixWorkspace_sptr &ws) override;
  void setPeriods(const Mantid::API::Workspace_sptr &ws) override;
  void setWsForMuonInfo(const std::string &filename) override;
  void setDirectoryChanged(bool hasDirectoryChanged) override;
  void setFilesToLoad(const std::vector<std::string> &files) override;

private:
  static std::string isCustomGroupingValid(const std::string &group, bool &isValid);
  static int extractRunNumber(const std::string &file);

  /// Last loaded data workspace
  Mantid::API::MatrixWorkspace_sptr m_loadedData;

  /// Number of detectors for current first run
  size_t m_numDetectors;

  // bool to state whether loading data
  std::atomic_bool m_loadingData;

  // Loading algorithm
  Mantid::API::IAlgorithm_sptr m_LoadingAlg;

  /// Flag for changes in watched directory
  std::atomic_bool m_directoryChanged;

  /// Last run loaded by auto
  int m_lastRunLoadedAuto;

  /// Files that are loaded
  std::vector<std::string> m_filesToLoad;

  /// Last run added by auto was addes as range
  std::atomic_bool m_wasLastAutoRange;

  /// Variables used to update available muon info
  Mantid::API::MatrixWorkspace_sptr m_wsForInfo;
  std::vector<std::string> m_periods;
  std::vector<std::string> m_logs;
  double m_minTime;
  std::string m_runsText;
};
} // namespace MantidQt::CustomInterfaces
