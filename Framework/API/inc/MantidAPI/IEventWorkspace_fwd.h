// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IEVENTWORKSPACE_FWD_H_
#define MANTID_API_IEVENTWORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::IEventWorkspace
*/

/// forward declare of Mantid::API::IEventWorkspace
class IEventWorkspace;
/// shared pointer to Mantid::API::IEventWorkspace
using IEventWorkspace_sptr = boost::shared_ptr<IEventWorkspace>;
/// shared pointer to Mantid::API::IEventWorkspace (const version)
using IEventWorkspace_const_sptr = boost::shared_ptr<const IEventWorkspace>;
/// unique pointer to Mantid::API::IEventWorkspace
using IEventWorkspace_uptr = std::unique_ptr<IEventWorkspace>;
/// unique pointer to Mantid::API::IEventWorkspace (const version)
using IEventWorkspace_const_uptr = std::unique_ptr<const IEventWorkspace>;

} // namespace API
} // namespace Mantid

#endif // MANTID_API_IEVENTWORKSPACE_FWD_H_
