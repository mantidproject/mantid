// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IPEAKWORKSPACE_FWD_H_
#define MANTID_API_IPEAKWORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::IPeaksWorkspace
*/

/// forward declare of Mantid::API::IPeaksWorkspace
class IPeaksWorkspace;
/// shared pointer to Mantid::API::IPeaksWorkspace
using IPeaksWorkspace_sptr = boost::shared_ptr<IPeaksWorkspace>;
/// shared pointer to Mantid::API::IPeaksWorkspace (const version)
using IPeaksWorkspace_const_sptr = boost::shared_ptr<const IPeaksWorkspace>;
/// unique pointer to Mantid::API::IPeaksWorkspace
using IPeaksWorkspace_uptr = std::unique_ptr<IPeaksWorkspace>;
/// unique pointer to Mantid::API::IPeaksWorkspace (const version)
using IPeaksWorkspace_const_uptr = std::unique_ptr<const IPeaksWorkspace>;
} // namespace API
} // namespace Mantid
#endif // MANTID_API_IPEAKWORKSPACE_FWD_H_
