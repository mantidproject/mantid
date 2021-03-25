// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/PreviewManager.h"

namespace Mantid::API {

std::vector<std::string>
PreviewManagerImpl::getPreviews(const std::string &facility,
                                const std::string &technique,
                                const std::string &acquisition) const {
  std::vector<std::string> previews;
  if (checkFacility(facility)) {
    for (const auto &facility_preview : m_previews.at(facility)) {
      for (const auto &technique_preview : facility_preview.second) {
        if (technique.empty() || facility_preview.first == technique) {
          for (const auto &acquisition_preview : technique_preview.second) {
            if (acquisition.empty() || technique_preview.first == acquisition) {
              previews.emplace_back(acquisition_preview.first);
            }
          }
        }
      }
    }
  }
  return previews;
}

const IPreview &PreviewManagerImpl::getPreview(
    const std::string &facility, const std::string &technique,
    const std::string &acquisition, const std::string &preview) const {
  if (!checkPreview(facility, technique, acquisition, preview)) {
    throw std::runtime_error(
        "Preview with the given name is not registered "
        "under the facility, technique and acquisition mode.");
  }
  return *m_previews.at(facility).at(technique).at(acquisition).at(preview);
}

bool PreviewManagerImpl::checkFacility(const std::string &facility) const {
  return m_previews.count(facility) > 0;
}
bool PreviewManagerImpl::checkTechnique(const std::string &facility,
                                        const std::string &technique) const {
  if (checkFacility(facility)) {
    return m_previews.at(facility).count(technique) > 0;
  } else {
    return false;
  }
}

bool PreviewManagerImpl::checkAcquisition(
    const std::string &facility, const std::string &technique,
    const std::string &acquisition) const {
  if (checkFacility(facility)) {
    if (checkTechnique(facility, technique))
      return m_previews.at(facility).at(technique).count(acquisition) > 0;
  }
  return false;
}

bool PreviewManagerImpl::checkPreview(const std::string &facility,
                                      const std::string &technique,
                                      const std::string &acquisition,
                                      const std::string &preview) const {
  if (checkAcquisition(facility, technique, acquisition)) {
    return m_previews.at(facility)
               .at(technique)
               .at(acquisition)
               .count(preview) > 0;
  }
  return false;
}

} // namespace Mantid::API
