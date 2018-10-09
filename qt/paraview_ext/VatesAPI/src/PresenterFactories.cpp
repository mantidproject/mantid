// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/PresenterFactories.h"

namespace Mantid {
namespace VATES {

const std::string &EmptyWorkspaceNamePolicy::getWorkspaceName(
    const Mantid::API::IMDWorkspace & /*workspace*/) {
  static std::string name{"__EmptyWorkspaceNamePolicy"};
  return name;
}

} // namespace VATES
} // namespace Mantid
