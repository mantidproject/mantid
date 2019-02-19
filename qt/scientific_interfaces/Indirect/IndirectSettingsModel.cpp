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

using namespace Mantid::Kernel;

namespace {

std::string::const_iterator
findNextNotCharacter(std::string::const_iterator fromIterator,
                     std::string::const_iterator toIterator,
                     std::string const &characters) {
  for (auto iter = fromIterator; iter < toIterator; ++iter)
    if (characters.find(*iter) == std::string::npos)
      return iter;
  return toIterator;
}

std::string::const_iterator
findNextCharacter(std::string::const_iterator fromIterator,
                  std::string::const_iterator toIterator,
                  std::string const &characters) {
  for (auto iter = fromIterator; iter < toIterator; ++iter)
    if (characters.find(*iter) != std::string::npos)
      return iter;
  return toIterator;
}

std::vector<std::string> splitBy(std::string::const_iterator fromIterator,
                                 std::string::const_iterator toIterator,
                                 std::string const &delimiters) {
  std::vector<std::string> substrings;
  auto iter = findNextNotCharacter(fromIterator, toIterator, delimiters);

  while (iter != toIterator) {
    auto const nextIter = findNextCharacter(iter, toIterator, delimiters);
    substrings.emplace_back(iter, nextIter);
    iter = findNextNotCharacter(nextIter, toIterator, delimiters);
  }
  return substrings;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSettingsModel::IndirectSettingsModel(
    std::string const &settingsGroup, std::string const &availableSettings)
    : m_settingsGroup(settingsGroup),
      m_settingsAvailable(
          splitBy(availableSettings.begin(), availableSettings.end(), ",")) {}

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
