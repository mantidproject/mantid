// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/PreviewManager.h"

namespace Mantid::API {

std::vector<std::string> PreviewManagerImpl::getPreviews(const std::string &facility,
                                                         const std::string &technique) const {
  std::vector<std::string> previews;
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility && (technique.empty() || preview->technique() == technique)) {
      previews.emplace_back(preview->name());
    }
  }
  return previews;
}

const IPreview &PreviewManagerImpl::getPreview(const std::string &facility, const std::string &technique,
                                               const std::string &preview_name) const {
  // cppcheck was unhappy about this for loop, so it got replaced, but is left as a comment so it is actually readable
  //  for (const auto &preview : m_previews) {
  //    if (preview->facility() == facility && preview->technique() == technique && preview->name() == preview_name) {
  //      return *preview;
  //    }
  //  }
  auto isPreview = [&](const auto &preview) {
    return (preview->facility() == facility && preview->technique() == technique && preview->name() == preview_name);
  };
  const auto result = std::find_if(std::begin(m_previews), std::end(m_previews), isPreview);

  if (result != std::end(m_previews)) {
    return *(*result).get();
  }
  throw std::runtime_error("Preview with the given name is not registered "
                           "under the facility and technique.");
}

bool PreviewManagerImpl::checkFacility(const std::string &facility) const {
  auto isFacility = [&](const auto &preview) { return (preview->facility() == facility); };
  return std::any_of(std::begin(m_previews), std::end(m_previews), isFacility);
}

bool PreviewManagerImpl::checkTechnique(const std::string &facility, const std::string &technique) const {
  auto isTechnique = [&](const auto &preview) {
    return (preview->facility() == facility && preview->technique() == technique);
  };
  return std::any_of(std::begin(m_previews), std::end(m_previews), isTechnique);
}

bool PreviewManagerImpl::checkPreview(const std::string &facility, const std::string &technique,
                                      const std::string &preview_name) const {
  auto isPreview = [&](const auto &preview) {
    return (preview->facility() == facility && preview->technique() == technique && preview->name() == preview_name);
  };
  return std::any_of(std::begin(m_previews), std::end(m_previews), isPreview);
}

} // namespace Mantid::API
