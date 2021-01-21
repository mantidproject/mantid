// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <vector>

namespace Mantid {
namespace DataObjects {

/** ScanningWorkspaceBuilder : This is a helper class to make it easy to build a
  scanning workspace (a workspace with moving detectors), where all the
  information about the scan is known in advance. The constructor takes the
  arguments for the basic construction, then checks are made for consistency as
  other information about the scanning workspace is set.

  Things that must be set for successful building:
   - Number of detectors, number of time indexes and number of bins (set via the
  constructor)
   - The instrument set via setInstrument
   - The time ranges set via setTimeRanges

  Some helper methods exist for specific cases, such as the whole instrument
  rotating around the sample.

  One current limitation to note here, that is not a general restriction within
  Mantid, is that every detector must have the same set of time indexes.
*/
class MANTID_DATAOBJECTS_DLL ScanningWorkspaceBuilder {
public:
  enum class IndexingType { Default, TimeOriented, DetectorOriented };

  ScanningWorkspaceBuilder(const std::shared_ptr<const Geometry::Instrument> &instrument, const size_t nTimeIndexes,
                           const size_t nBins, const bool isPointData = false);

  void setHistogram(HistogramData::Histogram histogram);

  void setTimeRanges(std::vector<std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>> timeRanges);
  void setTimeRanges(const Types::Core::DateAndTime &startTime, const std::vector<double> &durations);
  void setPositions(std::vector<std::vector<Kernel::V3D>> positions);
  void setRotations(std::vector<std::vector<Kernel::Quat>> rotations);
  void setRelativeRotationsForScans(const std::vector<double> relativeRotations, const Kernel::V3D &rotationPosition,
                                    const Kernel::V3D &rotationAxis);

  void setIndexingType(const IndexingType indexingType);

  API::MatrixWorkspace_sptr buildWorkspace() const;

private:
  size_t m_nDetectors;
  size_t m_nTimeIndexes;
  size_t m_nBins;

  std::shared_ptr<const Geometry::Instrument> m_instrument;

  HistogramData::Histogram m_histogram;

  std::vector<std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>> m_timeRanges;
  std::vector<std::vector<Kernel::V3D>> m_positions;
  std::vector<std::vector<Kernel::Quat>> m_rotations;

  std::vector<double> m_instrumentAngles;
  Kernel::V3D m_rotationAxis;
  Kernel::V3D m_rotationPosition;

  IndexingType m_indexingType;

  void buildOutputComponentInfo(Geometry::ComponentInfo &outputComponentInfo) const;

  void buildPositions(Geometry::DetectorInfo &outputDetectorInfo) const;
  void buildRotations(Geometry::DetectorInfo &outputDetectorInfo) const;
  void buildRelativeRotationsForScans(Geometry::DetectorInfo &outputDetectorInfo) const;

  void createTimeOrientedIndexInfo(API::MatrixWorkspace &ws) const;
  void createDetectorOrientedIndexInfo(API::MatrixWorkspace &ws) const;

  void verifyTimeIndexSize(const size_t timeIndexSize, const std::string &description) const;
  void verifyDetectorSize(const size_t detectorSize, const std::string &description) const;
  void validateInputs() const;
};

} // namespace DataObjects
} // namespace Mantid
