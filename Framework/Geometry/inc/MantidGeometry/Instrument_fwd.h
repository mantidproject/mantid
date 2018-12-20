#ifndef MANTID_GEOMETRY_INSTRUMENT_FWD_H_
#define MANTID_GEOMETRY_INSTRUMENT_FWD_H_

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
/**
  This file provides forward declarations for Mantid::Geometry::Instrument

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
