// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation BeamProfileFactory,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidAlgorithms/SampleCorrections/CircularBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/V3D.h"

namespace Mantid::Algorithms {
std::unique_ptr<IBeamProfile> BeamProfileFactory::createBeamProfile(const Geometry::Instrument &instrument,
                                                                    const API::Sample &sample) {
  const auto frame = instrument.getReferenceFrame();
  const auto source = instrument.getSource();

  const std::string beamShapeParam = source->getParameterAsString("beam-shape");
  if (beamShapeParam == "Slit") {
    const auto beamWidthParam = source->getNumberParameter("beam-width");
    const auto beamHeightParam = source->getNumberParameter("beam-height");
    if (beamWidthParam.size() == 1 && beamHeightParam.size() == 1) {
      return std::make_unique<RectangularBeamProfile>(*frame, source->getPos(), beamWidthParam[0], beamHeightParam[0]);
    }
  } else if (beamShapeParam == "Circle") {
    const auto beamRadiusParam = source->getNumberParameter("beam-radius");
    if (beamRadiusParam.size() == 1) {
      return std::make_unique<CircularBeamProfile>(*frame, source->getPos(), beamRadiusParam[0]);
    }
  }
  // revert to rectangular profile enclosing sample dimensions if no return by this point
  if (!sample.getShape().hasValidShape() && !sample.hasEnvironment()) {
    throw std::invalid_argument("Cannot determine beam profile without a sample shape and environment");
  }
  Kernel::V3D bbox;
  Kernel::V3D bboxCentre;
  if (sample.getShape().hasValidShape()) {
    bbox = sample.getShape().getBoundingBox().width();
    bboxCentre = sample.getShape().getBoundingBox().centrePoint();
  } else {
    bbox = sample.getEnvironment().boundingBox().width();
    bboxCentre = sample.getEnvironment().boundingBox().centrePoint();
  }
  // beam profile always centred on zero so set half width = centre + sample half width
  const double beamWidth = 2 * bboxCentre[frame->pointingHorizontal()] + bbox[frame->pointingHorizontal()];
  const double beamHeight = 2 * bboxCentre[frame->pointingUp()] + bbox[frame->pointingUp()];
  return std::make_unique<RectangularBeamProfile>(*frame, source->getPos(), beamWidth, beamHeight);
}
} // namespace Mantid::Algorithms
