// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_WORKSPACEGROUP_FWD_H_
#define MANTID_API_WORKSPACEGROUP_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::WorkspaceGroup
*/

/// forward declare of Mantid::API::WorkspaceGroup
class WorkspaceGroup;
/// shared pointer to Mantid::API::WorkspaceGroup
using WorkspaceGroup_sptr = boost::shared_ptr<WorkspaceGroup>;
/// shared pointer to Mantid::API::WorkspaceGroup, pointer to const version
using WorkspaceGroup_const_sptr = boost::shared_ptr<const WorkspaceGroup>;
/// unique pointer to Mantid::API::WorkspaceGroup
using WorkspaceGroup_uptr = std::unique_ptr<WorkspaceGroup>;
/// unique pointer to Mantid::API::WorkspaceGroup (const version)
using WorkspaceGroup_const_uptr = std::unique_ptr<const WorkspaceGroup>;

} // namespace API
} // namespace Mantid

#endif // MANTID_API_WORKSPACEGROUP_FWD_H_
