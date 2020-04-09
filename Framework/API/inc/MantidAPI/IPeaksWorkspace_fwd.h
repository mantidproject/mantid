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
  This file provides forward declarations for Mantid::API::IPeaksWorkspace
*/

/// forward declare of Mantid::API::IPeaksWorkspace
class IPeaksWorkspace;
/// shared pointer to Mantid::API::IPeaksWorkspace
using IPeaksWorkspace_sptr = std::shared_ptr<IPeaksWorkspace>;
/// shared pointer to Mantid::API::IPeaksWorkspace (const version)
using IPeaksWorkspace_const_sptr = std::shared_ptr<const IPeaksWorkspace>;
/// unique pointer to Mantid::API::IPeaksWorkspace
using IPeaksWorkspace_uptr = std::unique_ptr<IPeaksWorkspace>;
/// unique pointer to Mantid::API::IPeaksWorkspace (const version)
using IPeaksWorkspace_const_uptr = std::unique_ptr<const IPeaksWorkspace>;
} // namespace API
} // namespace Mantid
