// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_INSTRUMENT_FWD_H_
#define MANTID_GEOMETRY_INSTRUMENT_FWD_H_

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
/**
  This file provides forward declarations for Mantid::Geometry::Instrument
*/

/// Forward declare of Mantid::Geometry::Instrument
class Instrument;

/// Shared pointer to an instrument object
using Instrument_sptr = boost::shared_ptr<Instrument>;
/// Shared pointer to an const instrument object
using Instrument_const_sptr = boost::shared_ptr<const Instrument>;
/// unique pointer to an instrument
using Instrument_uptr = std::unique_ptr<Instrument>;
/// unique pointer to an instrument (const version)
using Instrument_const_uptr = std::unique_ptr<const Instrument>;

} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_INSTRUMENT_FWD_H_
