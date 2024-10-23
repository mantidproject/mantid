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
  void setLoadingData(bool isLoading);

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
};
} // namespace MantidQt::CustomInterfaces
