// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_IDETECTOR_FWD_H_
#define MANTID_GEOMETRY_IDETECTOR_FWD_H_

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
/**
  This file provides forward declarations for Mantid::Geometry::IDetector
*/

/// Forward declare of Mantid::Geometry::IDetector
class IDetector;

/// Shared pointer to an IDetector object
using IDetector_sptr = boost::shared_ptr<IDetector>;
/// Shared pointer to an const IDetector object
using IDetector_const_sptr = boost::shared_ptr<const IDetector>;
/// unique pointer to an IDetector
using IDetector_uptr = std::unique_ptr<IDetector>;
/// unique pointer to an IDetector (const version)
using IDetector_const_uptr = std::unique_ptr<const IDetector>;

} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_IDETECTOR_FWD_H_
