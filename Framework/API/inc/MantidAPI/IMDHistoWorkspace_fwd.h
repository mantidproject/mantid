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
  This file provides forward declarations for Mantid::API::IMDHistoWorkspace
*/

/// forward declare of Mantid::API::IMDHistoWorkspace
class IMDHistoWorkspace;
/// shared pointer to Mantid::API::IMDHistoWorkspace
using IMDHistoWorkspace_sptr = std::shared_ptr<IMDHistoWorkspace>;
/// shared pointer to Mantid::API::IMDHistoWorkspace (const version)
using IMDHistoWorkspace_const_sptr = std::shared_ptr<const IMDHistoWorkspace>;
/// unique pointer to Mantid::API::IMDHistoWorkspace
using IMDHistoWorkspace_uptr = std::unique_ptr<IMDHistoWorkspace>;
/// unique pointer to Mantid::API::IMDHistoWorkspace (const version)
using IMDHistoWorkspace_const_uptr = std::unique_ptr<const IMDHistoWorkspace>;

} // namespace API
} // namespace Mantid
