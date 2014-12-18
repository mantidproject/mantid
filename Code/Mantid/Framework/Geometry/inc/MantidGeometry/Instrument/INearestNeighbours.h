#ifndef MANTID_GEOMETRY_INSTRUMENT_INEARESTNEIGHBOURS_H
#define MANTID_GEOMETRY_INSTRUMENT_INEARESTNEIGHBOURS_H

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"
// Boost graphing
#ifndef Q_MOC_RUN
#include <boost/graph/adjacency_list.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

namespace Mantid {
namespace Geometry {
//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------
class Instrument;
class IComponent;

typedef boost::unordered_map<specid_t, std::set<detid_t>>
    ISpectrumDetectorMapping;

/**
 *  Abstract Nearest neighbours class. Implementations of this are used for
 *seaching for the nearest neighbours of a
 *  detector in the instrument geometry.
 *
 *  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 *National Laboratory & European Spallation Source
 *
 *  This file is part of Mantid.
 *
 *  Mantid is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mantid is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  File change history is stored at: <https://github.com/mantidproject/mantid>
 *  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_GEOMETRY_DLL INearestNeighbours {
public:
  /// Default (empty) destructor
  virtual ~INearestNeighbours(){};

  // Neighbouring spectra by radius
  virtual std::map<specid_t, Mantid::Kernel::V3D>
  neighboursInRadius(specid_t spectrum, double radius = 0.0) const = 0;

  // Neighbouring spectra by exact number of neighbours
  virtual std::map<specid_t, Mantid::Kernel::V3D>
  neighbours(specid_t spectrum) const = 0;
};
}
}
#endif
