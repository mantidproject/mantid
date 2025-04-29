// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "GUI/Preview/ROIType.h"
#include "Item.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "Reduction/ProcessingInstructions.h"

#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewRow : public Item {
public:
  explicit PreviewRow(const std::vector<std::string> &runNumbers);

  // These copy constructors are disabled as we need to pass by-ref
  // for call-backs to work through to the model
  PreviewRow(const PreviewRow &) = delete;
  PreviewRow &operator=(const PreviewRow &) = delete;

  // Allow moves as these are explicit
  PreviewRow(PreviewRow &&) = default;
  PreviewRow &operator=(PreviewRow &&) = default;

  std::vector<std::string> const &runNumbers() const;
  double theta() const;
  void setTheta(double theta);

  bool isGroup() const override;
  bool isPreview() const override;
  int totalItems() const override;
  int completedItems() const override;

  void renameOutputWorkspace(std::string const &, std::string const &) override {}
  void setOutputNames(std::vector<std::string> const &) override {}

  Mantid::API::MatrixWorkspace_sptr getLoadedWs() const noexcept;
  Mantid::API::MatrixWorkspace_sptr getSummedWs() const noexcept;
  Mantid::API::MatrixWorkspace_sptr getReducedWs() const noexcept;
  std::optional<ProcessingInstructions> getSelectedBanks() const noexcept;
  std::optional<ProcessingInstructions> getProcessingInstructions(ROIType regionType) const;

  void setLoadedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept;
  void setSummedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept;
  void setReducedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept;
  void setSelectedBanks(std::optional<ProcessingInstructions> selectedBanks) noexcept;
  void setProcessingInstructions(ROIType regionType, std::optional<ProcessingInstructions> processingInstructions);

  friend bool operator==(const PreviewRow &lhs, const PreviewRow &rhs) {
    // Note: This does not consider if the underlying item is equal currently
    return (&lhs == &rhs) || ((lhs.m_runNumbers == rhs.m_runNumbers) && (lhs.m_loadedWs == rhs.m_loadedWs) &&
                              (lhs.m_selectedBanks == rhs.m_selectedBanks));
  }

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  std::optional<ProcessingInstructions> m_selectedBanks{std::nullopt};
  std::optional<ProcessingInstructions> m_processingInstructions{std::nullopt};
  std::optional<ProcessingInstructions> m_backgroundProcessingInstructions{std::nullopt};
  std::optional<ProcessingInstructions> m_transmissionProcessingInstructions{std::nullopt};
  Mantid::API::MatrixWorkspace_sptr m_loadedWs;
  Mantid::API::MatrixWorkspace_sptr m_summedWs;
  Mantid::API::MatrixWorkspace_sptr m_reducedWs;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
