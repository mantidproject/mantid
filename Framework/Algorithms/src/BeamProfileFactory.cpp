// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation BeamProfileFactory,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidAlgorithms/SampleCorrections/CircularBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <cmath>

namespace Mantid::Algorithms {
std::unique_ptr<IBeamProfile> BeamProfileFactory::createBeamProfile(Geometry::ComponentInfo const &componentInfo,
                                                                    API::Sample const &sample) {
  auto const frame = componentInfo.referenceFrame();
  auto const source = componentInfo.source();
  auto const sourcePos = componentInfo.sourcePosition();

  auto const beamShapeParam = componentInfo.getStringParameter(source, "beam-shape");
  std::string const beamShape = beamShapeParam.empty() ? "" : beamShapeParam.front();
  if (beamShape == "Slit") {
    auto const beamWidthParam = componentInfo.getNumberParameter(source, "beam-width");
    auto const beamHeightParam = componentInfo.getNumberParameter(source, "beam-height");
    if (beamWidthParam.size() == 1 && beamHeightParam.size() == 1) {
      return std::make_unique<RectangularBeamProfile>(*frame, sourcePos, beamWidthParam[0], beamHeightParam[0]);
    }
  } else if (beamShape == "Circle") {
    auto const beamRadiusParam = componentInfo.getNumberParameter(source, "beam-radius");
    if (beamRadiusParam.size() == 1) {
      return std::make_unique<CircularBeamProfile>(*frame, sourcePos, beamRadiusParam[0]);
    }
  }
  // revert to rectangular profile enclosing sample dimensions if no return by this point
  if (!sample.getShape().hasValidShape() && !sample.hasEnvironment()) {
    throw std::invalid_argument("Cannot determine beam profile without a sample shape and environment");
  }
  Kernel::V3D bboxMin;
  Kernel::V3D bboxMax;
  if (sample.getShape().hasValidShape()) {
    bboxMin = sample.getShape().getBoundingBox().minPoint();
    bboxMax = sample.getShape().getBoundingBox().maxPoint();
  } else {
    bboxMin = sample.getEnvironment().boundingBox().minPoint();
    bboxMax = sample.getEnvironment().boundingBox().maxPoint();
  }
  // Beam profile is always centred on zero, so its half-extent must reach the furthest sample face
  // from the origin on either side. Using 2*max(|min|, |max|) keeps the beam valid when the sample
  // is offset and/or rotated such that its bbox lies entirely in a negative half-space.
  const auto hor = frame->pointingHorizontal();
  const auto up = frame->pointingUp();
  const double beamWidth = 2 * std::max(std::abs(bboxMin[hor]), std::abs(bboxMax[hor]));
  const double beamHeight = 2 * std::max(std::abs(bboxMin[up]), std::abs(bboxMax[up]));
  return std::make_unique<RectangularBeamProfile>(*frame, sourcePos, beamWidth, beamHeight);
}
} // namespace Mantid::Algorithms
