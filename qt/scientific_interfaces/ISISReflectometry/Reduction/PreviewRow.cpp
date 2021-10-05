// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PreviewRow.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewRow::PreviewRow(const std::vector<std::string> runNumbers) : Item(), m_runNumbers(std::move(runNumbers)) {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

std::vector<std::string> const &PreviewRow::runNumbers() const { return m_runNumbers; }

bool PreviewRow::isGroup() const { return false; }

bool PreviewRow::isPreview() const { return true; }

int PreviewRow::totalItems() const { return 1; }

int PreviewRow::completedItems() const { return 1; }

Mantid::API::MatrixWorkspace_sptr PreviewRow::getLoadedWs() const noexcept { return m_loadedWs; }

void PreviewRow::setLoadedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept { m_loadedWs = ws; }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry