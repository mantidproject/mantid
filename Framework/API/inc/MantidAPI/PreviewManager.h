// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IPreview.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidKernel/SingletonHolder.h"

#include <map>
#include <vector>

namespace Mantid {
namespace API {

/** PreviewManager : Manages the raw data previews
 */
class MANTID_API_DLL PreviewManagerImpl {
public:
  std::vector<std::string> getPreviews(const std::string &technique) {
    std::vector<std::string> previews;
    for (const auto &pv : m_previews[technique]) {
      previews.push_back(pv.first);
    }
    return previews;
  }
  const IPreview& getPreview(const std::string &technique,
                      const std::string &preview) {
    return *m_previews[technique][preview];
  }

  template <class T> void subscribe() {
    std::unique_ptr<Kernel::AbstractInstantiator<IPreview>> newI =
        std::make_unique<Kernel::Instantiator<T, IPreview>>();
    this->subscribe(std::move(newI));
  }

  template <class T> void
  subscribe(std::unique_ptr<Kernel::AbstractInstantiator<T>> instantiator) {
    const auto preview = instantiator->createUnwrappedInstance();
    const auto technique = preview->technique();
    const auto name = preview->name();
    m_previews[technique][name] = preview;
  }

private:
  std::map<std::string, std::map<std::string, IPreview*>> m_previews;
};

using PreviewManager = Mantid::Kernel::SingletonHolder<PreviewManagerImpl>;

} // namespace API
} // namespace Mantid

#define DECLARE_PREVIEW(classname)                                           \
  namespace {                                                                \
  Mantid::Kernel::RegistrationHelper register_preview_##classname((          \
      (Mantid::API::PreviewManager::Instance().subscribe<classname>()), 0)); \
  }
