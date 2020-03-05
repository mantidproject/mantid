// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "IndirectSettingsModel.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {

IndirectSettingsModel::IndirectSettingsModel() {}

void IndirectSettingsModel::setFacility(std::string const &facility) {
  auto const savedFacility = getFacility();
  if (savedFacility != facility)
    ConfigService::Instance().setFacility(facility);
}

std::string IndirectSettingsModel::getFacility() const {
  return ConfigService::Instance().getFacility().name();
}

} // namespace CustomInterfaces
} // namespace MantidQt
