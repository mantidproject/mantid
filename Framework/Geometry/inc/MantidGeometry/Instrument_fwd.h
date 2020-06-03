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
  This file provides forward declarations for Mantid::Geometry::Instrument
*/

/// Forward declare of Mantid::Geometry::Instrument
class Instrument;

/// Shared pointer to an instrument object
using Instrument_sptr = std::shared_ptr<Instrument>;
/// Shared pointer to an const instrument object
using Instrument_const_sptr = std::shared_ptr<const Instrument>;
/// unique pointer to an instrument
using Instrument_uptr = std::unique_ptr<Instrument>;
/// unique pointer to an instrument (const version)
using Instrument_const_uptr = std::unique_ptr<const Instrument>;

} // namespace Geometry
} // namespace Mantid
