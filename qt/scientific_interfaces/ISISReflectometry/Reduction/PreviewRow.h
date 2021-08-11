// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "Item.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewRow : public Item {
public:
  PreviewRow(std::vector<std::string> &runNumbers);
  std::vector<std::string> const &runNumbers() const;

  bool isGroup() const override;
  int totalItems() const override;
  int completedItems() const override;

  void renameOutputWorkspace(std::string const &, std::string const &) override {}
  void setOutputNames(std::vector<std::string> const &) override {}

private:
  std::vector<std::string> m_runNumbers;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry