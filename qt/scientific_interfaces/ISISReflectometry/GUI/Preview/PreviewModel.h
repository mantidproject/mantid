
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
#include "Reduction/PreviewRow.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IJobManager;

class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewModel final : public IPreviewModel {
public:
  PreviewModel();
  virtual ~PreviewModel() = default;

  bool loadWorkspaceFromAds(std::string const &workspaceName) override;
  void loadAndPreprocessWorkspaceAsync(std::string const &workspaceName, IJobManager &jobManager) override;
  void sumBanksAsync(IJobManager &jobManager) override;
  void reduceAsync(IJobManager &jobManager) override;

  std::string detIDsToString(std::vector<Mantid::detid_t> const &indices) const override;

  Mantid::API::MatrixWorkspace_sptr getLoadedWs() const override;
  std::vector<Mantid::detid_t> getSelectedBanks() const override;
  Mantid::API::MatrixWorkspace_sptr getSummedWs() const override;
  ProcessingInstructions getProcessingInstructions() const override;
  Mantid::API::MatrixWorkspace_sptr getReducedWs() const override;

  void setTheta(double theta) override;
  void setSelectedBanks(std::vector<Mantid::detid_t> selectedBanks) override;
  void setSelectedRegion(Selection const &selection) override;

  void exportSummedWsToAds() const override;
  void exportReducedWsToAds() const override;

private:
  // This should be an optional instead of a point, but we have issues reassigning it because boost::optional doesn't
  // play well with non-copyables. This should be fixable when we can use std::make_optional, but that is disabled on
  // Mac right now.
  std::unique_ptr<PreviewRow> m_runDetails{nullptr};

  void createRunDetails(std::string const &workspaceName);
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
