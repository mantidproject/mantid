#ifndef MANTID_API_DETECTORINFO_H_
#define MANTID_API_DETECTORINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>

#include <mutex>
#include <unordered_map>
#include <vector>

namespace Mantid {
using detid_t = int32_t;
namespace Beamline {
class DetectorInfo;
}
namespace Geometry {
class IComponent;
class IDetector;
class Instrument;
class ParameterMap;
}
namespace API {
class SpectrumInfo;

/** API::DetectorInfo is an intermediate step towards a DetectorInfo that is
  part of Instrument-2.0. The aim is to provide a nearly identical interface
  such that we can start refactoring existing code before the full-blown
  implementation of Instrument-2.0 is available.

  DetectorInfo provides easy access to commonly used parameters of individual
  detectors, such as mask and monitor flags, L1, L2, and 2-theta.

  This class is thread safe for read operations (const access) with OpenMP BUT
  NOT WITH ANY OTHER THREADING LIBRARY such as Poco threads or Intel TBB. There
  are no thread-safety guarantees for write operations (non-const access). Reads
  concurrent with writes or concurrent writes are not allowed.


  @author Simon Heybrock
  @date 2016

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
class MANTID_API_DLL DetectorInfo {
public:
  DetectorInfo(Beamline::DetectorInfo &detectorInfo,
               boost::shared_ptr<const Geometry::Instrument> instrument,
               boost::shared_ptr<std::vector<detid_t>> detectorIds,
               Geometry::ParameterMap *pmap,
               boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
                   detIdToIndexMap);

  DetectorInfo &operator=(const DetectorInfo &rhs);

  bool isEquivalent(const DetectorInfo &other) const;

  size_t size() const;
  bool isScanning() const;

  bool isMonitor(const size_t index) const;
  bool isMonitor(const std::pair<size_t, size_t> &index) const;
  bool isMasked(const size_t index) const;
  bool isMasked(const std::pair<size_t, size_t> &index) const;
  double l2(const size_t index) const;
  double l2(const std::pair<size_t, size_t> &index) const;
  double twoTheta(const size_t index) const;
  double twoTheta(const std::pair<size_t, size_t> &index) const;
  double signedTwoTheta(const size_t index) const;
  double signedTwoTheta(const std::pair<size_t, size_t> &index) const;
  Kernel::V3D position(const size_t index) const;
  Kernel::V3D position(const std::pair<size_t, size_t> &index) const;
  Kernel::Quat rotation(const size_t index) const;
  Kernel::Quat rotation(const std::pair<size_t, size_t> &index) const;

  void setMasked(const size_t index, bool masked);
  void setMasked(const std::pair<size_t, size_t> &index, bool masked);
  void clearMaskFlags();

  void setPosition(const size_t index, const Kernel::V3D &position);
  void setPosition(const std::pair<size_t, size_t> &index,
                   const Kernel::V3D &position);
  void setRotation(const size_t index, const Kernel::Quat &rotation);
  void setRotation(const std::pair<size_t, size_t> &index,
                   const Kernel::Quat &rotation);

  void setPosition(const Geometry::IComponent &comp, const Kernel::V3D &pos);
  void setRotation(const Geometry::IComponent &comp, const Kernel::Quat &rot);

  const Geometry::IDetector &detector(const size_t index) const;

  // This does not really belong into DetectorInfo, but it seems to be useful
  // while Instrument-2.0 does not exist.
  Kernel::V3D sourcePosition() const;
  Kernel::V3D samplePosition() const;
  double l1() const;

  const std::vector<detid_t> &detectorIDs() const;
  /// Returns the index of the detector with the given detector ID.
  /// This will throw an out of range exception if the detector does not exist.
  size_t indexOf(const detid_t id) const { return m_detIDToIndex->at(id); }

  size_t scanCount(const size_t index) const;
  std::pair<Kernel::DateAndTime, Kernel::DateAndTime>
  scanInterval(const std::pair<size_t, size_t> &index) const;
  void setScanInterval(
      const size_t index,
      const std::pair<Kernel::DateAndTime, Kernel::DateAndTime> &interval);

  void merge(const DetectorInfo &other);

  boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
  detIdToIndexMap() const;
  friend class SpectrumInfo;

private:
  const Geometry::IDetector &getDetector(const size_t index) const;
  boost::shared_ptr<const Geometry::IDetector>
  getDetectorPtr(const size_t index) const;
  const Geometry::IComponent &getSource() const;
  const Geometry::IComponent &getSample() const;
  const std::vector<size_t> &
  getAssemblyDetectorIndices(const Geometry::IComponent &comp) const;

  void cacheSource() const;
  void cacheSample() const;

  // These cache init functions are not thread-safe! Use only in combination
  // with std::call_once!
  void doCacheSource() const;
  void doCacheSample() const;
  void cacheL1() const;

  /// Reference to the actual DetectorInfo object (non-wrapping part).
  Beamline::DetectorInfo &m_detectorInfo;

  Geometry::ParameterMap *m_pmap;
  boost::shared_ptr<const Geometry::Instrument> m_instrument;
  boost::shared_ptr<const std::vector<detid_t>> m_detectorIDs;
  boost::shared_ptr<const std::unordered_map<detid_t, size_t>> m_detIDToIndex;
  // The following variables are mutable, since they are initialized (cached)
  // only on demand, by const getters.
  mutable boost::shared_ptr<const Geometry::IComponent> m_source;
  mutable boost::shared_ptr<const Geometry::IComponent> m_sample;
  mutable bool m_sourceGood{false};
  mutable bool m_sampleGood{false};
  mutable Kernel::V3D m_sourcePos;
  mutable Kernel::V3D m_samplePos;
  mutable double m_L1;
  mutable std::once_flag m_sourceCached;
  mutable std::once_flag m_sampleCached;
  mutable std::once_flag m_L1Cached;

  mutable std::vector<boost::shared_ptr<const Geometry::IDetector>>
      m_lastDetector;
  mutable std::vector<
      std::pair<const Geometry::IComponent *, std::vector<size_t>>>
      m_lastAssemblyDetectorIndices;
  mutable std::vector<size_t> m_lastIndex;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_DETECTORINFO_H_ */
