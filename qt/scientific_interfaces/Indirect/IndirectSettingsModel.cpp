// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "IndirectSettingsModel.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;

namespace {

template <typename T, typename Predicate>
void removeElementsIf(T &iterable, Predicate const &filter) {
  auto const iter = std::remove_if(iterable.begin(), iterable.end(), filter);
  if (iter != iterable.end())
    iterable.erase(iter, iterable.end());
}

std::vector<std::string> splitBy(std::string const &str,
                                 std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  removeElementsIf(subStrings, [](std::string const &subString) {
    return subString.empty();
  });
  return subStrings;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSettingsModel::IndirectSettingsModel(
    std::string const &settingsGroup, std::string const &availableSettings)
    : m_settingsGroup(settingsGroup),
      m_settingsAvailable(splitBy(availableSettings, ",")) {}

std::string IndirectSettingsModel::getSettingsGroup() const {
  return m_settingsGroup;
}

bool IndirectSettingsModel::hasInterfaceSettings() const {
  return !m_settingsAvailable.empty();
}

bool IndirectSettingsModel::isSettingAvailable(
    std::string const &settingName) const {
  return std::find(m_settingsAvailable.begin(), m_settingsAvailable.end(),
                   settingName) != m_settingsAvailable.end();
}

void IndirectSettingsModel::setFacility(std::string const &facility) {
  auto const savedFacility = getFacility();
  if (savedFacility != facility)
    ConfigService::Instance().setFacility(facility);
}

std::string IndirectSettingsModel::getFacility() const {
  return ConfigService::Instance().getFacility().name();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
