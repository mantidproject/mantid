#ifndef MANTID_DETECTOR_SEARCHER_H_
#define MANTID_DETECTOR_SEARCHER_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/NearestNeighbours.h"
#include "MantidKernel/V3D.h"

#include <Eigen/Core>

/**
  DetectorSearcher is a helper class to find a specific detector within
  the instrument geometry.

  This class solves the problem of finding a detector given a Qlab vector. Two
  search strategies are used depending on the instrument's geometry.

  1) For rectangular detector geometries the InstrumentRayTracer class is used
  to recursively search the instrument tree.

  2) For geometries which do not use rectangular detectors ray tracing to every
  component is very expensive. In this case it is quicker to use a
  NearestNeighbours search to find likely detector positions.

  @author Samuel Jackson
  @date 2017

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid {
namespace API {

class MANTID_API_DLL DetectorSearcher {
public:
  /// Search result type representing whether a detector was found and if so
  /// which detector index it was.
  using DetectorSearchResult = std::tuple<bool, size_t>;

  /// Create a new DetectorSearcher with the given instrument & detectors
  DetectorSearcher(Geometry::Instrument_const_sptr instrument,
                   const Geometry::DetectorInfo &detInfo);
  /// Find a detector that intsects with the given Qlab vector
  DetectorSearchResult findDetectorIndex(const Kernel::V3D &q);

private:
  /// Attempt to find a detector using a full instrument ray tracing strategy
  DetectorSearchResult searchUsingInstrumentRayTracing(const Kernel::V3D &q);
  /// Attempt to find a detector using a nearest neighbours search strategy
  DetectorSearchResult searchUsingNearestNeighbours(const Kernel::V3D &q);
  /// Check whether the given direction in detector space intercepts with a
  /// detector
  std::tuple<bool, size_t> checkInteceptWithNeighbours(
      const Kernel::V3D &direction,
      const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours)
      const;
  /// Helper function to build the nearest neighbour tree
  void createDetectorCache();
  /// Helper function to convert a Qlab vector to a direction in detector space
  Kernel::V3D convertQtoDirection(const Kernel::V3D &q) const;
  /// Helper function to handle the tube gap parameter in tube instruments
  DetectorSearchResult handleTubeGap(
      const Kernel::V3D &detectorDir,
      const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours);

  // Instance variables

  /// flag for whether to use InstrumentRayTracer or NearestNeighbours
  const bool m_usingFullRayTrace;
  /// flag for whether the crystallography convention is to be used
  const double m_crystallography_convention;
  /// detector info for the instrument
  const Geometry::DetectorInfo &m_detInfo;
  /// handle to the instrument to search for detectors in
  Geometry::Instrument_const_sptr m_instrument;
  /// vector of detector indicies used in the search
  std::vector<size_t> m_indexMap;
  /// Detector search cache for fast look-up of detectors
  std::unique_ptr<Kernel::NearestNeighbours<3>> m_detectorCacheSearch;
  /// instrument ray tracer object for searching in rectangular detectors
  std::unique_ptr<Geometry::InstrumentRayTracer> m_rayTracer;
};
} // namespace API
} // namespace Mantid

#endif
