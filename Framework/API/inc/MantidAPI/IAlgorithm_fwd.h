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
  This file provides forward declarations for Mantid::API::IAlgorithm
*/

/// forward declare of Mantid::API::IAlgorithm
class IAlgorithm;
/// shared pointer to Mantid::API::IAlgorithm
using IAlgorithm_sptr = std::shared_ptr<IAlgorithm>;
/// shared pointer to Mantid::API::IAlgorithm (const version)
using IAlgorithm_const_sptr = std::shared_ptr<const IAlgorithm>;
/// unique pointer to Mantid::API::IAlgorithm
using IAlgorithm_uptr = std::unique_ptr<IAlgorithm>;
/// unique pointer to Mantid::API::IAlgorithm (const version)
using IAlgorithm_const_uptr = std::unique_ptr<const IAlgorithm>;

} // namespace API
} // namespace Mantid
