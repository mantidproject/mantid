// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_WORKSPACE_FWD_H_
#define MANTID_API_WORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::Workspace
*/

/// forward declare of Mantid::API::Workspace
class Workspace;
/// shared pointer to Mantid::API::Workspace
using Workspace_sptr = boost::shared_ptr<Workspace>;
/// shared pointer to Mantid::API::Workspace (const version)
using Workspace_const_sptr = boost::shared_ptr<const Workspace>;
/// unique pointer to Mantid::API::Workspace
using Workspace_uptr = std::unique_ptr<Workspace>;
/// unique pointer to Mantid::API::Workspace (const version)
using Workspace_const_uptr = std::unique_ptr<const Workspace>;

} // namespace API
} // namespace Mantid

#endif // MANTID_API_WORKSPACE_FWD_H_
