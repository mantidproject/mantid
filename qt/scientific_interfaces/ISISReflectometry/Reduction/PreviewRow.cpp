// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PreviewRow.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

PreviewRow::PreviewRow(std::vector<std::string> &runNumbers) : Item(), m_runNumbers(runNumbers) {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

std::vector<std::string> const &PreviewRow::runNumbers() const { return m_runNumbers; }

bool PreviewRow::isGroup() const { return false; }

int PreviewRow::totalItems() const { return 1; }

int PreviewRow::completedItems() const { return 1; }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry