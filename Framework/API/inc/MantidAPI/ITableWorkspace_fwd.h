// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ITABLEWORKSPACE_FWD_H_
#define MANTID_API_ITABLEWORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::ITableWorkspace
*/

/// forward declare of Mantid::API::ITableWorkspace
class ITableWorkspace;
/// shared pointer to Mantid::API::ITableWorkspace
using ITableWorkspace_sptr = boost::shared_ptr<ITableWorkspace>;
/// shared pointer to Mantid::API::ITableWorkspace (const version)
using ITableWorkspace_const_sptr = boost::shared_ptr<const ITableWorkspace>;
/// unique pointer to Mantid::API::ITableWorkspace
using ITableWorkspace_uptr = std::unique_ptr<ITableWorkspace>;
/// unique pointer to Mantid::API::ITableWorkspace (const version)
using ITableWorkspace_const_uptr = std::unique_ptr<const ITableWorkspace>;

} // namespace API
} // Namespace Mantid
#endif // MANTID_API_ITABLEWORKSPACE_FWD_H_
