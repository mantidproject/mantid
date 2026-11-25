// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/BeamlineBuilder.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include <memory>

namespace Mantid::Geometry::ComponentHelper {
using Kernel::V3D;

namespace {
/// A source, a sample and point detectors (each a 1 cm sphere) in the given frame.
Geometry::Instrument_sptr makeVirtualInstrument(Kernel::V3D const &sourcePos, Kernel::V3D const &samplePos,
                                                std::vector<Kernel::V3D> const &vecdetpos,
                                                std::vector<detid_t> const &vecdetid,
                                                std::shared_ptr<const ReferenceFrame> referenceFrame) {
  BeamlineBuilder builder;
  auto const root = builder.addAssembly(BeamlineBuilder::noParent, "");
  builder.setReferenceFrame(std::move(referenceFrame));
  builder.setSource(builder.addObjComponent(root, "source", sourcePos, Kernel::Quat(),
                                            createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1")));
  builder.setSample(builder.addComponent(root, "some-surface-holder", samplePos));
  for (size_t i = 0; i < vecdetpos.size(); ++i) {
    // FIXME - should be cubi... pixel
    builder.addDetector(root, "point-detector", vecdetid[i] /*detector id*/, vecdetpos[i], Kernel::Quat(),
                        createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
  }
  return Instrument::fromBeamline(builder.build());
}
} // namespace

/**
 * createOneDetectorInstrument, creates the most simple possible definition of
 *an instrument in which we can extract a valid L1 and L2 distance for unit
 *calculations.
 *
 * Beam direction is along X,
 * Up direction is Y
 *
 * @param sourcePos : V3D position
 * @param samplePos : V3D sample position
 * @param detectorPos : V3D detector position
 * @return Instrument generated.
 */
Geometry::Instrument_sptr createMinimalInstrument(const Mantid::Kernel::V3D &sourcePos,
                                                  const Mantid::Kernel::V3D &samplePos,
                                                  const Mantid::Kernel::V3D &detectorPos) {
  return makeVirtualInstrument(
      sourcePos, samplePos, {detectorPos}, {1},
      std::make_shared<ReferenceFrame>(Mantid::Geometry::Y /*up*/, Mantid::Geometry::X /*along*/, Left, "0,0,0"));
}

Geometry::Instrument_sptr createVirtualInstrument(Kernel::V3D sourcePos, Kernel::V3D samplePos,
                                                  const std::vector<Kernel::V3D> &vecdetpos,
                                                  const std::vector<detid_t> &vecdetid) {
  return makeVirtualInstrument(
      sourcePos, samplePos, vecdetpos, vecdetid,
      std::make_shared<ReferenceFrame>(Mantid::Geometry::Y /*up*/, Mantid::Geometry::Z /*along*/, Right, "0,0,0"));
}

/**
 * Create a sphere object
 */
std::shared_ptr<CSGObject> createSphere(double radius, const V3D &centre, const std::string &id) {
  ShapeFactory shapeMaker;
  return shapeMaker.createShape(sphereXML(radius, centre, id));
}

/**
 * Return the XML for a sphere.
 */
std::string sphereXML(double radius, const Kernel::V3D &centre, const std::string &id) {
  std::ostringstream xml;
  xml << "<sphere id=\"" << id << "\">"
      << "<centre x=\"" << centre.X() << "\"  y=\"" << centre.Y() << "\" z=\"" << centre.Z() << "\" />"
      << "<radius val=\"" << radius << "\" />"
      << "</sphere>";
  return xml.str();
}

} // namespace Mantid::Geometry::ComponentHelper
