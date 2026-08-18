// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/BoundedValidator.h"
#include <algorithm>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AnyShapeAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using Mantid::Algorithms::BeamProfileFactory;

AnyShapeAbsorption::AnyShapeAbsorption() : AbsorptionCorrection(), m_cubeSide(0.0) {}

void AnyShapeAbsorption::defineProperties() {
  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, "The size of one side of an integration element cube in mm");
}

/// Fetch the properties and set the appropriate member variables
void AnyShapeAbsorption::retrieveProperties() {
  m_cubeSide = getProperty("ElementSize"); // in mm
  m_cubeSide *= 0.001;                     // now in m
}

std::string AnyShapeAbsorption::sampleXML() {
  // Returning an empty string signals to the base class that it should
  // use the object already attached to the sample.
  return std::string();
}

/// How much of the workspace's goniometer rotation the sample shape has not had applied to it.
///
/// A workspace can arrive with its sample shape in either frame. CopySample bakes the destination's
/// goniometer into the shape definition, so the shape comes back already in the lab frame, while
/// SetGoniometer on its own leaves the shape untouched in its own frame. Both leave the same
/// goniometer on the run, so the run alone cannot tell them apart: applying R unconditionally would
/// rotate an already-rotated shape a second time, and applying nothing ignores the rotation
/// altogether. With B baked into the shape and the run reporting R, what is left to apply is R*B^-1 -
/// the identity when the shape is already in the lab frame, and R when it is not. This is the same
/// reconciliation EstimateScatteringVolumeCentreOfMass makes, so the two agree on where the sample is.
Kernel::Matrix<double> AnyShapeAbsorption::outstandingSampleRotation() const {
  const auto gonioR = m_inputWS->run().getGoniometer().getR();
  Kernel::Matrix<double> alreadyApplied = m_sampleObject->getAppliedRotation();
  if (alreadyApplied == Kernel::Matrix<double>(3, 3, true)) {
    return gonioR;
  }
  g_log.information("The sample shape already carries a rotation, so only the remainder of the "
                    "goniometer rotation is applied");
  alreadyApplied.Invert();
  return gonioR * alreadyApplied;
}

Kernel::V3D AnyShapeAbsorption::toShapeFrame(const Kernel::V3D &labFramePoint) const {
  return m_shapeIsInLabFrame ? labFramePoint : m_labToShapeFrame * labFramePoint;
}

/// Calculate the distances for L1 and element size for each element in the
/// sample
void AnyShapeAbsorption::initialiseCachedDistances() {
  const auto rotation = outstandingSampleRotation();
  m_shapeIsInLabFrame = (rotation == Kernel::Matrix<double>(3, 3, true));
  m_labToShapeFrame = rotation;
  if (!m_shapeIsInLabFrame) {
    m_labToShapeFrame.Invert();
  }

  // First, check if a 'gauge volume' has been defined. If not, it's the same as
  // the sample.
  IObject_const_sptr integrationVolume;
  // A gauge volume and a beam are described in the lab frame; the sample is only there if something
  // has rotated it. Which of the two we end up with decides how the raster has to be done below.
  bool volumeIsInLabFrame = true;
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  } else {
    try {
      auto beamProfile = BeamProfileFactory::createBeamProfile(*m_inputWS->getInstrument(), Mantid::API::Sample());
      integrationVolume = beamProfile->getIntersectionWithSample(*m_sampleObject);
    } catch (const std::invalid_argument &) {
      // If createBeamProfile fails, the beam parameters are not defined
      // If getIntersectionWithSample fails, the beam misses the object
      // In either case we will just fall back to using the whole sample below.
    }
    if (integrationVolume == nullptr) {
      // If the beam profile is not defined, use the sample object
      integrationVolume = IObject_const_sptr(m_sampleObject->clone());
      volumeIsInLabFrame = false; // it *is* the sample, so it shares the sample's frame whatever that is
    }
  }

  Geometry::Raster raster;
  if (m_shapeIsInLabFrame) {
    // Everything is in one frame, which is the case for every workspace without a goniometer.
    raster = Geometry::Rasterize::calculate(m_beamDirection, *integrationVolume, *m_sampleObject, m_cubeSide);
  } else if (volumeIsInLabFrame) {
    // Dice the region in the lab frame and map each candidate voxel back into the sample's frame to
    // be tested and traced. Dicing it where it was defined keeps its bounding box tight; rotating the
    // region instead would inflate the box and admit voxels outside the region itself.
    raster = Geometry::Rasterize::calculateInLabFrame(m_beamDirection, *integrationVolume, *m_sampleObject, m_cubeSide,
                                                      rotation);
  } else {
    // The region is the sample, so dice it in the sample's frame - with the beam direction rotated in
    // to match, since that is a lab frame vector - and rotate the accepted positions out afterwards.
    raster =
        Geometry::Rasterize::calculate(toShapeFrame(m_beamDirection), *integrationVolume, *m_sampleObject, m_cubeSide);
    std::transform(raster.position.cbegin(), raster.position.cend(), raster.position.begin(),
                   [&rotation](const auto &positionInShapeFrame) { return rotation * positionInShapeFrame; });
  }

  m_sampleVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_numVolumeElements = raster.l1.size();
  m_L1s = std::move(raster.l1);
  m_elementPositions = std::move(raster.position);
  m_elementVolumes = std::move(raster.volume);
}

/// The elements are in the lab frame and the sample shape may not be, so undo the difference before
/// tracing. Both ends of the ray need it: rotating only the element would aim it somewhere else.
void AnyShapeAbsorption::calculateDistances(const Geometry::IDetector &detector, std::vector<double> &L2s) const {
  if (m_shapeIsInLabFrame) {
    AbsorptionCorrection::calculateDistances(detector, L2s);
    return;
  }
  const auto detectorPos = toShapeFrame(detectorPositionToTraceTo(detector));
  for (size_t i = 0; i < m_numVolumeElements; ++i) {
    const auto elementPos = toShapeFrame(m_elementPositions[i]);
    Geometry::Track outgoing(elementPos, normalize(detectorPos - elementPos));
    m_sampleObject->interceptSurface(outgoing);
    L2s[i] = outgoing.totalDistInsideObject();
  }
}

std::shared_ptr<const Geometry::IObject> AnyShapeAbsorption::constructGaugeVolume() const {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  std::shared_ptr<const Geometry::IObject> volume =
      ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
}

} // namespace Mantid::Algorithms
