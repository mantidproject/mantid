// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "DllConfig.h"
#include "IALCDataLoadingView.h"

namespace MantidQt::CustomInterfaces {

class MANTIDQT_MUONINTERFACE_DLL IALCDataLoadingModel {

public:
  virtual ~IALCDataLoadingModel() = default;
  virtual void load(const IALCDataLoadingView *view) = 0;
  virtual void cancelLoading() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr exportWorkspace() = 0;
  virtual bool checkCustomGrouping(const std::string &detGroupingType, const std::string &forwardGrouping,
                                   const std::string &backwardGrouping) = 0;
  virtual void updateAutoLoadCancelled() = 0;
  virtual bool loadFilesFromWatchingDirectory(const std::string &firstFile, const std::vector<std::string> &files,
                                              const std::string &runsText) = 0;
  virtual std::string getPathFromFiles(const std::vector<std::string> &files) const = 0;

  // Getters
  virtual bool getLoadingData() const = 0;
  virtual double getMinTime() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getLoadedData() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getWsForMuonInfo() const = 0;
  virtual std::vector<std::string> &getLogs() = 0;
  virtual std::vector<std::string> &getPeriods() = 0;
  virtual std::string &getRunsText() = 0;

  // Setters
  virtual void setLoadingData(bool isLoading) = 0;
  virtual void setLoadedData(const Mantid::API::MatrixWorkspace_sptr &data) = 0;
  virtual void setLogs(const Mantid::API::MatrixWorkspace_sptr &ws) = 0;
  virtual void setPeriods(const Mantid::API::Workspace_sptr &ws) = 0;
  virtual void setWsForMuonInfo(const std::string &filename) = 0;
  virtual void setDirectoryChanged(bool hasDirectoryChanged) = 0;
  virtual void setFilesToLoad(const std::vector<std::string> &files) = 0;
};
} // namespace MantidQt::CustomInterfaces
