// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_DETECTORINFO_H_
#define MANTID_GEOMETRY_DETECTORINFO_H_

#include <boost/shared_ptr.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
using detid_t = int32_t;
namespace Beamline {
class DetectorInfo;
}
namespace API {
class SpectrumInfo;
}
namespace Geometry {
class IDetector;
class Instrument;

/** Geometry::DetectorInfo is an intermediate step towards a DetectorInfo that
  is part of Instrument-2.0. The aim is to provide a nearly identical interface
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
*/
class MANTID_GEOMETRY_DLL DetectorInfo {
public:
  DetectorInfo(std::unique_ptr<Beamline::DetectorInfo> detectorInfo,
               boost::shared_ptr<const Geometry::Instrument> instrument,
               boost::shared_ptr<const std::vector<detid_t>> detectorIds,
               boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
                   detIdToIndexMap);
  DetectorInfo(const DetectorInfo &other);
  DetectorInfo &operator=(const DetectorInfo &rhs);
  ~DetectorInfo();

  bool isEquivalent(const DetectorInfo &other) const;

  size_t size() const;
  size_t scanSize() const;
  bool isScanning() const;

  bool isMonitor(const size_t index) const;
  bool isMonitor(const std::pair<size_t, size_t> &index) const;
  bool isMasked(const size_t index) const;
  bool isMasked(const std::pair<size_t, size_t> &index) const;
  bool hasMaskedDetectors() const;
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

  const Geometry::IDetector &detector(const size_t index) const;

  // This does not really belong into DetectorInfo, but it seems to be useful
  // while Instrument-2.0 does not exist.
  Kernel::V3D sourcePosition() const;
  Kernel::V3D samplePosition() const;
  double l1() const;

  const std::vector<detid_t> &detectorIDs() const;
  /// Returns the index of the detector with the given detector ID.
  /// This will throw an out of range exception if the detector does not exist.
  size_t indexOf(const detid_t id) const;

  size_t scanCount() const;
  const std::vector<
      std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>>
  scanIntervals() const;

  friend class API::SpectrumInfo;
  friend class Instrument;

  DetectorInfoIterator<DetectorInfo> begin();
  DetectorInfoIterator<DetectorInfo> end();
  const DetectorInfoIterator<const DetectorInfo> cbegin() const;
  const DetectorInfoIterator<const DetectorInfo> cend() const;

private:
  const Geometry::IDetector &getDetector(const size_t index) const;
  boost::shared_ptr<const Geometry::IDetector>
  getDetectorPtr(const size_t index) const;

  /// Pointer to the actual DetectorInfo object (non-wrapping part).
  std::unique_ptr<Beamline::DetectorInfo> m_detectorInfo;

  boost::shared_ptr<const Geometry::Instrument> m_instrument;
  boost::shared_ptr<const std::vector<detid_t>> m_detectorIDs;
  boost::shared_ptr<const std::unordered_map<detid_t, size_t>> m_detIDToIndex;

  mutable std::vector<boost::shared_ptr<const Geometry::IDetector>>
      m_lastDetector;
  mutable std::vector<size_t> m_lastIndex;
};

using DetectorInfoIt = DetectorInfoIterator<DetectorInfo>;
using DetectorInfoConstIt = DetectorInfoIterator<const DetectorInfo>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFO_H_ */
