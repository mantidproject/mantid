// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Objects/IObject.h"

#include <array>
#include <tuple>
#include <utility>

namespace Mantid {
namespace Algorithms {
class DetectorGridDefinition;
}
namespace Geometry {
class ReferenceFrame;
}
namespace HistogramData {
class Histogram;
}
namespace Kernel {
class V3D;
}
namespace Algorithms {
/**
  Defines functions and utilities to create and deal with sparse instruments.
*/

class MANTID_ALGORITHMS_DLL SparseWorkspace : public DataObjects::Workspace2D {
public:
  SparseWorkspace(const API::MatrixWorkspace &modelWS, const size_t wavelengthPoints, const size_t rows,
                  const size_t columns);
  virtual HistogramData::Histogram interpolateFromDetectorGrid(const double lat, const double lon) const;
  virtual HistogramData::Histogram bilinearInterpolateFromDetectorGrid(const double lat, const double lon) const;

protected:
  SparseWorkspace(const SparseWorkspace &other);
  std::unique_ptr<Algorithms::DetectorGridDefinition> m_gridDef;
  static std::array<double, 4> inverseDistanceWeights(const std::array<double, 4> &distances);
  static double greatCircleDistance(const double lat1, const double long1, const double lat2, const double long2);
  Mantid::Geometry::IObject_sptr makeCubeShape();
  static HistogramData::Histogram modelHistogram(const API::MatrixWorkspace &modelWS, const size_t wavelengthPoints);
  static std::tuple<double, double> extremeWavelengths(const API::MatrixWorkspace &ws);
  static std::tuple<double, double, double, double> extremeAngles(const API::MatrixWorkspace &ws);
  HistogramData::HistogramY secondDerivative(const std::array<size_t, 3> &indices, const double distanceStep) const;
  HistogramData::HistogramE esq(const HistogramData::HistogramE &e) const;
  HistogramData::HistogramE esqrt(HistogramData::HistogramE e) const;

private:
  SparseWorkspace *doClone() const override;
};

/// unique pointer to Mantid::API::SparseWorkspace
using SparseWorkspace_uptr = std::unique_ptr<SparseWorkspace>;
using SparseWorkspace_sptr = std::shared_ptr<SparseWorkspace>;

} // namespace Algorithms
} // namespace Mantid
