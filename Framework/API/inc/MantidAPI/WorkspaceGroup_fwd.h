// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::WorkspaceGroup
*/

/// forward declare of Mantid::API::WorkspaceGroup
class WorkspaceGroup;
/// shared pointer to Mantid::API::WorkspaceGroup
using WorkspaceGroup_sptr = std::shared_ptr<WorkspaceGroup>;
/// shared pointer to Mantid::API::WorkspaceGroup, pointer to const version
using WorkspaceGroup_const_sptr = std::shared_ptr<const WorkspaceGroup>;
/// unique pointer to Mantid::API::WorkspaceGroup
using WorkspaceGroup_uptr = std::unique_ptr<WorkspaceGroup>;
/// unique pointer to Mantid::API::WorkspaceGroup (const version)
using WorkspaceGroup_const_uptr = std::unique_ptr<const WorkspaceGroup>;

} // namespace API
} // namespace Mantid
