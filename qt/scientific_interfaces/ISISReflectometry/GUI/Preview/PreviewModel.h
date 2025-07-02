
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "ROIType.h"
#include "Reduction/PreviewRow.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IJobManager;

class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewModel final : public IPreviewModel {
public:
  PreviewModel();
  ~PreviewModel() override = default;

  bool loadWorkspaceFromAds(std::string const &workspaceName) override;
  void loadAndPreprocessWorkspaceAsync(std::string const &workspaceName, IJobManager &jobManager) override;
  void sumBanksAsync(IJobManager &jobManager) override;
  void reduceAsync(IJobManager &jobManager) override;

  Mantid::API::MatrixWorkspace_sptr getLoadedWs() const override;
  std::optional<ProcessingInstructions> getSelectedBanks() const override;
  Mantid::API::MatrixWorkspace_sptr getSummedWs() const override;
  std::optional<ProcessingInstructions> getProcessingInstructions(ROIType regionType) const override;
  Mantid::API::MatrixWorkspace_sptr getReducedWs() const override;
  std::optional<double> getDefaultTheta() const override;
  PreviewRow const &getPreviewRow() const override;
  std::optional<Selection> const getSelectedRegion(ROIType regionType) override;

  void setLoadedWs(Mantid::API::MatrixWorkspace_sptr workspace);
  void setSummedWs(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void setTheta(double theta) override;
  void setSelectedBanks(std::optional<ProcessingInstructions> selectedBanks) override;
  void setSelectedRegion(ROIType regionType, Selection const &selection) override;

  void exportSummedWsToAds() const override;
  void exportReducedWsToAds() const override;

private:
  std::unique_ptr<PreviewRow> m_runDetails{nullptr};

  void createRunDetails(std::string const &workspaceName);

  std::optional<double> getThetaFromLogs(std::string const &logName) const;

  void setProcessingInstructions(ROIType regionType, ProcessingInstructions processingInstructions);
  void setSelectedRegionMembers(ROIType regionType, Selection const &selection);

  std::optional<Selection> m_selectedSignalRegion;
  std::optional<Selection> m_selectedBackgroundRegion;
  std::optional<Selection> m_selectedTransmissionRegion;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
