// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/PreviewManager.h"

namespace Mantid {
namespace API {

std::vector<std::string>
PreviewManagerImpl::getPreviews(const std::string &facility,
                                const std::string &technique) {
  std::vector<std::string> previews;
  for (const auto &pvs : m_previews[facility]) {
    for (const auto &pv : pvs.second) {
      if (technique.empty() || pvs.first == technique) {
        previews.push_back(pv.first);
      }
    }
  }
  return previews;
}

const IPreview &PreviewManagerImpl::getPreview(const std::string &facility,
                                               const std::string &technique,
                                               const std::string &preview) {
  if (!checkPreview(facility, technique, preview)) {
    throw std::runtime_error("Preview with the given name is not registered "
                             "under the facility and technique.");
  }
  return *m_previews[facility][technique][preview];
}

bool PreviewManagerImpl::checkFacility(const std::string &facility) {
  return m_previews.count(facility) > 0;
}
bool PreviewManagerImpl::checkTechnique(const std::string &facility,
                                        const std::string &technique) {
  if (checkFacility(facility)) {
    return m_previews[facility].count(technique) > 0;
  } else {
    return false;
  }
}
bool PreviewManagerImpl::checkPreview(const std::string &facility,
                                      const std::string &technique,
                                      const std::string &preview) {
  if (checkTechnique(facility, technique)) {
    return m_previews[facility][technique].count(preview) > 0;
  } else {
    return false;
  }
}

} // namespace API
} // namespace Mantid
