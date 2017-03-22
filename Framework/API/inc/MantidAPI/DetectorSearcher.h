#ifndef MANTID_DETECTOR_SEARCHER_H_
#define MANTID_DETECTOR_SEARCHER_H_

#include "MantidKernel/V3D.h"
#include "MantidKernel/NearestNeighbours.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"

#include <Eigen/Core>

/**
  DetectorSearcher is a helper class to find a specific detector within
  the instrument geometry.

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

class DetectorSearcher {

public:
  DetectorSearcher(Geometry::Instrument_const_sptr instrument, const DetectorInfo& info);
  const std::tuple<bool, size_t> findDetectorIndex(const Kernel::V3D& q);

private:
  void createDetectorCache();
  const std::tuple<bool, size_t> searchUsingInstrumentRayTracing(const Kernel::V3D& q);
  const std::tuple<bool, size_t> searchUsingNearestNeighbours(const Kernel::V3D& q);
  const std::tuple<bool, size_t> checkInteceptWithNeighbours(const Kernel::V3D& direction,
                                                             const Kernel::NearestNeighbours<3>::NearestNeighbourResults& neighbours) const;
  Kernel::V3D convertQtoDirection(const Kernel::V3D& q) const;

  // Instance variables

  const bool m_usingFullRayTrace;
  const bool m_crystallography_convention;
  const DetectorInfo& m_detInfo;
  Geometry::Instrument_const_sptr m_instrument;
  /// vector of detector indicies used in the search
  std::vector<size_t> m_indexMap;
  /// Detector search cache for fast look-up of detectors
  std::unique_ptr<Kernel::NearestNeighbours<3>> m_detectorCacheSearch;
  /// instrument ray tracer object for searching in rectangular detectors
  std::unique_ptr<Geometry::InstrumentRayTracer> m_rayTracer;
};

}
}

#endif
