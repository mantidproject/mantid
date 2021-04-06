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
  This file provides forward declarations for Mantid::API::IFunction
*/

/// forward declare of Mantid::API::IFunction
class IFunction;
/// Shared pointer to Mantid::API::IFunction
using IFunction_sptr = std::shared_ptr<IFunction>;
/// Shared pointer to Mantid::API::IFunction (const version)
using IFunction_const_sptr = std::shared_ptr<const IFunction>;

/// forward declare of Mantid::API::CompositeFunction
class CompositeFunction;
/// Shared pointer to Mantid::API::CompositeFunction
using CompositeFunction_sptr = std::shared_ptr<CompositeFunction>;
/// Shared pointer to Mantid::API::CompositeFunction (const version)
using CompositeFunction_const_sptr = std::shared_ptr<const CompositeFunction>;

/// forward declare of Mantid::API::MultiDomainFunction
class MultiDomainFunction;
/// Shared pointer to Mantid::API::MultiDomainFunction
using MultiDomainFunction_sptr = std::shared_ptr<MultiDomainFunction>;
/// Shared pointer to Mantid::API::MultiDomainFunction (const version)
using MultiDomainFunction_const_sptr = std::shared_ptr<const MultiDomainFunction>;

} // namespace API
} // namespace Mantid
