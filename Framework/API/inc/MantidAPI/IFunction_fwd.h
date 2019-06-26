// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IFUNCTION_FWD_H_
#define IFUNCTION_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {

/**
  This file provides forward declarations for Mantid::API::IFunction
*/

/// forward declare of Mantid::API::IFunction
class IFunction;
/// Shared pointer to Mantid::API::IFunction
using IFunction_sptr = boost::shared_ptr<IFunction>;
/// Shared pointer to Mantid::API::IFunction (const version)
using IFunction_const_sptr = boost::shared_ptr<const IFunction>;

/// forward declare of Mantid::API::CompositeFunction
class CompositeFunction;
/// Shared pointer to Mantid::API::CompositeFunction
using CompositeFunction_sptr = boost::shared_ptr<CompositeFunction>;
/// Shared pointer to Mantid::API::CompositeFunction (const version)
using CompositeFunction_const_sptr = boost::shared_ptr<const CompositeFunction>;

/// forward declare of Mantid::API::MultiDomainFunction
class MultiDomainFunction;
/// Shared pointer to Mantid::API::MultiDomainFunction
using MultiDomainFunction_sptr = boost::shared_ptr<MultiDomainFunction>;
/// Shared pointer to Mantid::API::MultiDomainFunction (const version)
using MultiDomainFunction_const_sptr =
    boost::shared_ptr<const MultiDomainFunction>;

} // namespace API
} // namespace Mantid

#endif /* IFUNCTION_FWD_H_ */
