// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL SettingsModel {
public:
  SettingsModel();
  virtual ~SettingsModel() = default;

  virtual void setFacility(std::string const &facility);
  virtual std::string getFacility() const;
};

} // namespace CustomInterfaces
} // namespace MantidQt
