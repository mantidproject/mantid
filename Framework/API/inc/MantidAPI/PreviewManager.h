// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IPreview.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidKernel/SingletonHolder.h"

#include <map>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace Mantid {
namespace API {

using PreviewRegister = std::set<IPreview_uptr>;

/** PreviewManager : Manages the raw data previews.
 */
class MANTID_API_DLL PreviewManagerImpl {
public:
  PreviewManagerImpl() = default;
  PreviewManagerImpl(const PreviewManagerImpl &) = delete;
  PreviewManagerImpl &operator=(const PreviewManagerImpl &) = delete;
  std::vector<std::string> getPreviews(const std::string &facility, const std::string &technique = "",
                                       const std::string &acquisition = "", const std::string &geometry = "") const;
  const IPreview &getPreview(const std::string &facility, const std::string &technique, const std::string &acquisition,
                             const std::string &geometry, const std::string &preview) const;
  template <class T> void subscribe() {
    static_assert(std::is_base_of<IPreview, T>::value);
    IPreview_uptr preview = std::make_unique<T>();
    const auto facility = preview->facility();
    const auto technique = preview->technique();
    const auto acquisition = preview->acquisition();
    const auto geometry = preview->geometry();
    const auto name = preview->name();
    if (checkPreview(facility, technique, acquisition, geometry, name)) {
      throw std::runtime_error("Preview with the same name is already registered for the same "
                               "facility, technique and acquisition mode.");
    }
    m_previews.insert(std::move(preview));
  }

private:
  bool checkFacility(const std::string &facility) const;
  bool checkTechnique(const std::string &facility, const std::string &technique) const;
  bool checkAcquisition(const std::string &facility, const std::string &technique,
                        const std::string &acquisition) const;
  bool checkGeometry(const std::string &facility, const std::string &technique, const std::string &acquisition,
                     const std::string &geometry) const;
  bool checkPreview(const std::string &facility, const std::string &technique, const std::string &acquisition,
                    const std::string &geometry, const std::string &preview) const;
  PreviewRegister m_previews;
};

using PreviewManager = Mantid::Kernel::SingletonHolder<PreviewManagerImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::PreviewManagerImpl>;
}
} // namespace Mantid

#define DECLARE_PREVIEW(classname)                                                                                     \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper                                                                                   \
      register_preview_##classname(((Mantid::API::PreviewManager::Instance().subscribe<classname>()), 0));             \
  }
