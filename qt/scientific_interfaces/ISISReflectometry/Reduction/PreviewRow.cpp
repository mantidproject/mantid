// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PreviewRow.h"
#include "GUI/Preview/ROIType.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDTypes.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewRow::PreviewRow(const std::vector<std::string> &runNumbers)
    : Item(), m_runNumbers(std::move(runNumbers)), m_theta{0.0} {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

std::vector<std::string> const &PreviewRow::runNumbers() const { return m_runNumbers; }

double PreviewRow::theta() const { return m_theta; }

void PreviewRow::setTheta(double theta) { m_theta = theta; }

bool PreviewRow::isGroup() const { return false; }

bool PreviewRow::isPreview() const { return true; }

int PreviewRow::totalItems() const { return 1; }

int PreviewRow::completedItems() const { return 1; }

Mantid::API::MatrixWorkspace_sptr PreviewRow::getLoadedWs() const noexcept { return m_loadedWs; }
Mantid::API::MatrixWorkspace_sptr PreviewRow::getSummedWs() const noexcept { return m_summedWs; }
Mantid::API::MatrixWorkspace_sptr PreviewRow::getReducedWs() const noexcept { return m_reducedWs; }

void PreviewRow::setLoadedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept { m_loadedWs = std::move(ws); }
void PreviewRow::setSummedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept { m_summedWs = std::move(ws); }
void PreviewRow::setReducedWs(Mantid::API::MatrixWorkspace_sptr ws) noexcept { m_reducedWs = std::move(ws); }

std::optional<ProcessingInstructions> PreviewRow::getSelectedBanks() const noexcept { return m_selectedBanks; }

void PreviewRow::setSelectedBanks(std::optional<ProcessingInstructions> selectedBanks) noexcept {
  m_selectedBanks = std::move(selectedBanks);
}

std::optional<ProcessingInstructions> PreviewRow::getProcessingInstructions(ROIType regionType) const {
  switch (regionType) {
  case ROIType::Signal:
    return m_processingInstructions;
  case ROIType::Background:
    return m_backgroundProcessingInstructions;
  case ROIType::Transmission:
    return m_transmissionProcessingInstructions;
  }
  throw std::invalid_argument("Unexpected ROIType provided");
}

void PreviewRow::setProcessingInstructions(ROIType regionType,
                                           std::optional<ProcessingInstructions> processingInstructions) {
  switch (regionType) {
  case ROIType::Signal:
    m_processingInstructions = std::move(processingInstructions);
    return;
  case ROIType::Background:
    m_backgroundProcessingInstructions = std::move(processingInstructions);
    return;
  case ROIType::Transmission:
    m_transmissionProcessingInstructions = std::move(processingInstructions);
    return;
  }
  throw std::invalid_argument("Unexpected ROIType provided");
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
