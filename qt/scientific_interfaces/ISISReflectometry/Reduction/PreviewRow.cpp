// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PreviewRow.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDTypes.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewRow::PreviewRow(const std::vector<std::string> &runNumbers) : Item(), m_runNumbers(std::move(runNumbers)) {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

std::vector<std::string> const &PreviewRow::runNumbers() const { return m_runNumbers; }

bool PreviewRow::isGroup() const { return false; }

bool PreviewRow::isPreview() const { return true; }

int PreviewRow::totalItems() const { return 1; }

int PreviewRow::completedItems() const { return 1; }

Mantid::API::MatrixWorkspace_sptr PreviewRow::getLoadedWs() const noexcept { return m_loadedWs; }
Mantid::API::MatrixWorkspace_sptr PreviewRow::getSummedWs() const noexcept { return m_summedWs; }

void PreviewRow::setLoadedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept { m_loadedWs = std::move(ws); }
void PreviewRow::setSummedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept { m_summedWs = std::move(ws); }

std::vector<Mantid::detid_t> PreviewRow::getSelectedBanks() const noexcept { return m_selectedBanks; }

void PreviewRow::setSelectedBanks(std::vector<Mantid::detid_t> selectedBanks) noexcept {
  m_selectedBanks = std::move(selectedBanks);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
