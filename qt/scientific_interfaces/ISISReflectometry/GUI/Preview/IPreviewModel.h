// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "ROIType.h"
#include "Reduction/PreviewRow.h"
#include "Reduction/ProcessingInstructions.h"

#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IJobManager;

class IPreviewModel {
public:
  using Selection = std::vector<double>;

  IPreviewModel() = default;
  virtual ~IPreviewModel() = default;
  virtual bool loadWorkspaceFromAds(std::string const &workspaceName) = 0;
  virtual void loadAndPreprocessWorkspaceAsync(std::string const &workspaceName, IJobManager &jobManager) = 0;
  virtual void sumBanksAsync(IJobManager &jobManager) = 0;
  virtual void reduceAsync(IJobManager &jobManager) = 0;

  virtual Mantid::API::MatrixWorkspace_sptr getLoadedWs() const = 0;
  virtual std::optional<ProcessingInstructions> getSelectedBanks() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getSummedWs() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getReducedWs() const = 0;
  virtual std::optional<ProcessingInstructions> getProcessingInstructions(ROIType regionType) const = 0;
  virtual std::optional<double> getDefaultTheta() const = 0;
  virtual PreviewRow const &getPreviewRow() const = 0;
  virtual std::optional<Selection> const getSelectedRegion(ROIType regionType) = 0;

  virtual void setSummedWs(Mantid::API::MatrixWorkspace_sptr workspace) = 0;

  virtual void setTheta(double theta) = 0;

  virtual void setSelectedBanks(std::optional<ProcessingInstructions> selectedBanks) = 0;
  virtual void setSelectedRegion(ROIType regionType, Selection const &selection) = 0;

  virtual void exportSummedWsToAds() const = 0;
  virtual void exportReducedWsToAds() const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
