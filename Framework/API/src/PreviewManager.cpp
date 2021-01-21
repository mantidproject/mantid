// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/PreviewManager.h"

namespace Mantid {
namespace API {

std::vector<std::string> PreviewManagerImpl::getPreviews(const std::string &facility,
                                                         const std::string &technique) const {
  std::vector<std::string> previews;
  if (m_previews.find(facility) != m_previews.end()) {
    for (const auto &pvs : m_previews.at(facility)) {
      for (const auto &pv : pvs.second) {
        if (technique.empty() || pvs.first == technique) {
          previews.emplace_back(pv.first);
        }
      }
    }
  }
  return previews;
}

const IPreview &PreviewManagerImpl::getPreview(const std::string &facility, const std::string &technique,
                                               const std::string &preview) const {
  if (!checkPreview(facility, technique, preview)) {
    throw std::runtime_error("Preview with the given name is not registered "
                             "under the facility and technique.");
  }
  return *m_previews.at(facility).at(technique).at(preview);
}

bool PreviewManagerImpl::checkFacility(const std::string &facility) const { return m_previews.count(facility) > 0; }
bool PreviewManagerImpl::checkTechnique(const std::string &facility, const std::string &technique) const {
  if (checkFacility(facility)) {
    return m_previews.at(facility).count(technique) > 0;
  } else {
    return false;
  }
}
bool PreviewManagerImpl::checkPreview(const std::string &facility, const std::string &technique,
                                      const std::string &preview) const {
  if (checkTechnique(facility, technique)) {
    return m_previews.at(facility).at(technique).count(preview) > 0;
  } else {
    return false;
  }
}

} // namespace API
} // namespace Mantid
