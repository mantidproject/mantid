// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
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

class MANTID_ALGORITHMS_DLL SparseInstrument {
public:
  std::tuple<double, double> extremeWavelengths(const API::MatrixWorkspace &ws);
  HistogramData::Histogram modelHistogram(const API::MatrixWorkspace &modelWS,
                                          const size_t wavelengthPoints);
  API::MatrixWorkspace_uptr
  createSparseWS(const API::MatrixWorkspace &modelWS,
                 const Algorithms::DetectorGridDefinition &grid,
                 const size_t wavelengthPoints);
  std::unique_ptr<const Algorithms::DetectorGridDefinition>
  createDetectorGridDefinition(const API::MatrixWorkspace &modelWS,
                               const size_t rows, const size_t columns);

private:
  Mantid::Geometry::IObject_sptr makeCubeShape();
};

} // namespace Algorithms
} // namespace Mantid
