// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IALGORITHM_FWD_H_
#define MANTID_API_IALGORITHM_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::IAlgorithm
*/

/// forward declare of Mantid::API::IAlgorithm
class IAlgorithm;
/// shared pointer to Mantid::API::IAlgorithm
using IAlgorithm_sptr = boost::shared_ptr<IAlgorithm>;
/// shared pointer to Mantid::API::IAlgorithm (const version)
using IAlgorithm_const_sptr = boost::shared_ptr<const IAlgorithm>;
/// unique pointer to Mantid::API::IAlgorithm
using IAlgorithm_uptr = std::unique_ptr<IAlgorithm>;
/// unique pointer to Mantid::API::IAlgorithm (const version)
using IAlgorithm_const_uptr = std::unique_ptr<const IAlgorithm>;

} // namespace API
} // namespace Mantid

#endif // MANTID_API_IALGORITHM_FWD_H_
