// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IMDHISTOWORKSPACE_FWD_H_
#define MANTID_API_IMDHISTOWORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::IMDHistoWorkspace
*/

/// forward declare of Mantid::API::IMDHistoWorkspace
class IMDHistoWorkspace;
/// shared pointer to Mantid::API::IMDHistoWorkspace
using IMDHistoWorkspace_sptr = boost::shared_ptr<IMDHistoWorkspace>;
/// shared pointer to Mantid::API::IMDHistoWorkspace (const version)
using IMDHistoWorkspace_const_sptr = boost::shared_ptr<const IMDHistoWorkspace>;
/// unique pointer to Mantid::API::IMDHistoWorkspace
using IMDHistoWorkspace_uptr = std::unique_ptr<IMDHistoWorkspace>;
/// unique pointer to Mantid::API::IMDHistoWorkspace (const version)
using IMDHistoWorkspace_const_uptr = std::unique_ptr<const IMDHistoWorkspace>;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IMDHISTOWORKSPACE_FWD_H_ */
