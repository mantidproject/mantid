// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "Item.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"

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

  bool isGroup() const override;
  bool isPreview() const override;
  int totalItems() const override;
  int completedItems() const override;

  void renameOutputWorkspace(std::string const &, std::string const &) override {}
  void setOutputNames(std::vector<std::string> const &) override {}

  Mantid::API::MatrixWorkspace_sptr getLoadedWs() const noexcept;
  Mantid::API::MatrixWorkspace_sptr getSummedWs() const noexcept;
  std::vector<Mantid::detid_t> getSelectedBanks() const noexcept;

  void setLoadedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept;
  void setSummedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept;
  void setSelectedBanks(std::vector<Mantid::detid_t> selectedBanks) noexcept;

  friend bool operator==(const PreviewRow &lhs, const PreviewRow &rhs) {
    // Note: This does not consider if the underlying item is equal currently
    return (&lhs == &rhs) || ((lhs.m_runNumbers == rhs.m_runNumbers) && (lhs.m_loadedWs == rhs.m_loadedWs) &&
                              (lhs.m_selectedBanks == rhs.m_selectedBanks));
  }

private:
  std::vector<std::string> m_runNumbers;
  std::vector<Mantid::detid_t> m_selectedBanks;
  Mantid::API::MatrixWorkspace_sptr m_loadedWs;
  Mantid::API::MatrixWorkspace_sptr m_summedWs;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
