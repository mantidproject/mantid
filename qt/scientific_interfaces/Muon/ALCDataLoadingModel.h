// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IALCDataLoadingView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include <atomic>

namespace MantidQt::CustomInterfaces {

class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingModel {

public:
  ALCDataLoadingModel();
  ~ALCDataLoadingModel() = default;
  void load(const std::vector<std::string> &files, const IALCDataLoadingView *view);
  void cancelLoading() const;
  void setLoadingData(bool isLoading);
  bool getLoadingData();
  void setLoadedData(const Mantid::API::MatrixWorkspace_sptr &data);
  size_t getNumDetectors() const;
  Mantid::API::MatrixWorkspace_sptr getLoadedData();
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();
  void setWsForMuonInfo(const std::string &filename);
  void setLogs(Mantid::API::MatrixWorkspace_sptr);
  void setPeriods(Mantid::API::Workspace_sptr);
  std::vector<std::string> getLogs();
  std::vector<std::string> getPeriods();
  Mantid::API::MatrixWorkspace_sptr getWsForMuonInfo();
  double getMinTime();
  int extractRunNumber(const std::string &file);

private:
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
  std::vector<std::string> m_filesLoaded;

  /// Last run added by auto was addes as range
  std::atomic_bool m_wasLastAutoRange;

  /// Previous first run number (INSTNAMERUNNUMBER)
  std::string m_previousFirstRun;

  /// Variables using to update available info
  Mantid::API::MatrixWorkspace_sptr m_wsForInfo;
  std::vector<std::string> m_periods;
  std::vector<std::string> m_logs;
  double m_minTime;
};
} // namespace MantidQt::CustomInterfaces
