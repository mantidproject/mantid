// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SPECTRUMINFO_H_
#define MANTID_API_SPECTRUMINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/SpectrumInfoIterator.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/cow_ptr.h"

#include <boost/shared_ptr.hpp>

#include <vector>

namespace Mantid {
using detid_t = int32_t;
class SpectrumDefinition;
namespace Beamline {
class SpectrumInfo;
}
namespace Geometry {
class DetectorInfo;
class IDetector;
class Instrument;
class ParameterMap;
} // namespace Geometry
namespace API {
class ExperimentInfo;

/** API::SpectrumInfo is an intermediate step towards a SpectrumInfo that is
  part of Instrument-2.0. The aim is to provide a nearly identical interface
  such that we can start refactoring existing code before the full-blown
  implementation of Instrument-2.0 is available.

  SpectrumInfo provides easy access to commonly used parameters of individual
  spectra (which may correspond to one or more detectors), such as mask and
  monitor flags, L1, L2, and 2-theta.

  This class is thread safe for read operations (const access) with OpenMP BUT
  NOT WITH ANY OTHER THREADING LIBRARY such as Poco threads or Intel TBB. There
  are no thread-safety guarantees for write operations (non-const access). Reads
  concurrent with writes or concurrent writes are not allowed.


  @author Simon Heybrock
  @date 2016
*/
class MANTID_API_DLL SpectrumInfo {
public:
  SpectrumInfo(const Beamline::SpectrumInfo &spectrumInfo,
               const ExperimentInfo &experimentInfo,
               Geometry::DetectorInfo &detectorInfo);
  ~SpectrumInfo();

  size_t size() const;

  const SpectrumDefinition &spectrumDefinition(const size_t index) const;
  const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
  sharedSpectrumDefinitions() const;

  bool isMonitor(const size_t index) const;
  bool isMasked(const size_t index) const;
  double l2(const size_t index) const;
  double twoTheta(const size_t index) const;
  double signedTwoTheta(const size_t index) const;
  double azimuthal(const size_t index) const;
  Kernel::V3D position(const size_t index) const;
  bool hasDetectors(const size_t index) const;
  bool hasUniqueDetector(const size_t index) const;

  void setMasked(const size_t index, bool masked);

  // This is likely to be deprecated/removed with the introduction of
  // Instrument-2.0: The concept of detector groups will probably be dropped so
  // returning a single detector for a spectrum will not be possible anymore.
  const Geometry::IDetector &detector(const size_t index) const;

  // This does not really belong into SpectrumInfo, but it seems to be useful
  // while Instrument-2.0 does not exist.
  Kernel::V3D sourcePosition() const;
  Kernel::V3D samplePosition() const;
  double l1() const;

  SpectrumInfoIterator<SpectrumInfo> begin();
  SpectrumInfoIterator<SpectrumInfo> end();
  const SpectrumInfoIterator<const SpectrumInfo> cbegin() const;
  const SpectrumInfoIterator<const SpectrumInfo> cend() const;

  friend class ExperimentInfo;

private:
  const Geometry::IDetector &getDetector(const size_t index) const;
  const SpectrumDefinition &
  checkAndGetSpectrumDefinition(const size_t index) const;

  const ExperimentInfo &m_experimentInfo;
  Geometry::DetectorInfo &m_detectorInfo;
  const Beamline::SpectrumInfo &m_spectrumInfo;
  mutable std::vector<boost::shared_ptr<const Geometry::IDetector>>
      m_lastDetector;
  mutable std::vector<size_t> m_lastIndex;
};

using SpectrumInfoIt = SpectrumInfoIterator<SpectrumInfo>;
using SpectrumInfoConstIt = SpectrumInfoIterator<const SpectrumInfo>;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRUMINFO_H_ */
