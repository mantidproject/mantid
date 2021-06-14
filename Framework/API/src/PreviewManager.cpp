// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/PreviewManager.h"

namespace Mantid::API {

std::vector<std::string> PreviewManagerImpl::getPreviews(const std::string &facility, const std::string &technique,
                                                         const std::string &acquisition) const {
  std::vector<std::string> previews;
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility && (technique.empty() || preview->technique() == technique) &&
        (acquisition.empty() || preview->acquisition() == acquisition)) {
      previews.emplace_back(preview->name());
    }
  }
  return previews;
}

const IPreview &PreviewManagerImpl::getPreview(const std::string &facility, const std::string &technique,
                                               const std::string &acquisition, const std::string &preview_name) const {
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility && preview->technique() == technique && preview->acquisition() == acquisition &&
        preview->name() == preview_name) {
      return *preview;
    }
  }
  throw std::runtime_error("Preview with the given name is not registered "
                           "under the facility, technique and acquisition mode.");
}

bool PreviewManagerImpl::checkFacility(const std::string &facility) const {
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility) {
      return true;
    }
  }
  return false;
}

bool PreviewManagerImpl::checkTechnique(const std::string &facility, const std::string &technique) const {
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility && preview->technique() == technique) {
      return true;
    }
  }
  return false;
}

bool PreviewManagerImpl::checkAcquisition(const std::string &facility, const std::string &technique,
                                          const std::string &acquisition) const {
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility && preview->technique() == technique && preview->acquisition() == acquisition) {
      return true;
    }
  }
  return false;
}

bool PreviewManagerImpl::checkPreview(const std::string &facility, const std::string &technique,
                                      const std::string &acquisition,
                                      const std::string &preview_name) const {
  for (const auto &preview : m_previews) {
    if (preview->facility() == facility && preview->technique() == technique && preview->acquisition() == acquisition &&
        preview->name() == preview_name) {
      return true;
    }
  }
  return false;
}

} // namespace Mantid::API
