// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace Geometry {
/**
  This file provides forward declarations for Mantid::Geometry::IDetector
*/

/// Forward declare of Mantid::Geometry::IDetector
class IDetector;

/// Shared pointer to an IDetector object
using IDetector_sptr = std::shared_ptr<IDetector>;
/// Shared pointer to an const IDetector object
using IDetector_const_sptr = std::shared_ptr<const IDetector>;
/// unique pointer to an IDetector
using IDetector_uptr = std::unique_ptr<IDetector>;
/// unique pointer to an IDetector (const version)
using IDetector_const_uptr = std::unique_ptr<const IDetector>;

} // namespace Geometry
} // namespace Mantid
