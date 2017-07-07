#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/Detector.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
namespace ComponentHelper {
using Kernel::V3D;
using Kernel::Quat;

/**
 * Add/modify an entry in the parameter map for the given component
 * to update its position. The component is const
 * as the move doesn't actually change the object
 * @param comp A reference to the component to move
 * @param pmap A reference to the ParameterMap that will hold the new position
 * @param pos The new position
 * @param positionType Defines how the given position should be interpreted @see
 * TransformType enumeration
 */
void moveComponent(const IComponent &comp, ParameterMap &pmap,
                   const Kernel::V3D &pos, const TransformType positionType) {
  //
  // This behaviour was copied from how MoveInstrumentComponent worked
  //

  // First set it to the new absolute position
  V3D newPos = pos;
  switch (positionType) {
  case Absolute: // Do nothing
    break;
  case Relative:
    newPos += comp.getPos();
    break;
  default:
    throw std::invalid_argument("moveComponent -  Unknown positionType: " +
                                std::to_string(positionType));
  }

  // Then find the corresponding relative position
  auto parent = comp.getParent();
  if (parent) {
    newPos -= parent->getPos();
    Quat rot = parent->getRotation();
    rot.inverse();
    rot.rotate(newPos);
  }

  // Add a parameter for the new position
  pmap.addV3D(comp.getComponentID(), "pos", newPos);
}

/**
 * Add/modify an entry in the parameter map for the given component
 * to update its rotation. The component is const
 * as the move doesn't actually change the object
 * @param comp A reference to the component to move
 * @param pmap A reference to the ParameterMap that will hold the new position
 * @param rot The rotation quaternion
 * @param rotType Defines how the given rotation should be interpreted @see
 * TransformType enumeration
 */
void rotateComponent(const IComponent &comp, ParameterMap &pmap,
                     const Kernel::Quat &rot, const TransformType rotType) {
  //
  // This behaviour was copied from how RotateInstrumentComponent worked
  //

  Quat newRot = rot;
  if (rotType == Absolute) {
    // Find the corresponding relative position
    auto parent = comp.getParent();
    if (parent) {
      Quat rot0 = parent->getRotation();
      rot0.inverse();
      newRot = rot0 * rot;
    }
  } else if (rotType == Relative) {
    const Quat &Rot0 = comp.getRelativeRot();
    newRot = Rot0 * rot;
  } else {
    throw std::invalid_argument("rotateComponent -  Unknown rotType: " +
                                std::to_string(rotType));
  }

  // Add a parameter for the new rotation
  pmap.addQuat(comp.getComponentID(), "rot", newRot);
}

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
Geometry::Instrument_sptr
createMinimalInstrument(const Mantid::Kernel::V3D &sourcePos,
                        const Mantid::Kernel::V3D &samplePos,
                        const Mantid::Kernel::V3D &detectorPos) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      Mantid::Geometry::Y /*up*/, Mantid::Geometry::X /*along*/, Left,
      "0,0,0"));

  // A source
  ObjComponent *source = new ObjComponent("source");
  source->setPos(sourcePos);
  source->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
  instrument->add(source);
  instrument->markAsSource(source);

  // A sample
  ObjComponent *sample = new ObjComponent("some-surface-holder");
  sample->setPos(samplePos);
  sample->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
  instrument->add(sample);
  instrument->markAsSamplePos(sample);

  // A detector
  Detector *det = new Detector("point-detector", 1 /*detector id*/, nullptr);
  det->setPos(detectorPos);
  det->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
  instrument->add(det);
  instrument->markAsDetector(det);

  return instrument;
}

Geometry::Instrument_sptr
createVirtualInstrument(Kernel::V3D sourcePos, Kernel::V3D samplePos,
                        const std::vector<Kernel::V3D> &vecdetpos,
                        const std::vector<detid_t> &vecdetid) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      Mantid::Geometry::Y /*up*/, Mantid::Geometry::Z /*along*/, Right,
      "0,0,0"));

  // A source
  ObjComponent *source = new ObjComponent("source");
  source->setPos(sourcePos);
  source->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
  instrument->add(source);
  instrument->markAsSource(source);

  // A sample
  ObjComponent *sample = new ObjComponent("some-surface-holder");
  sample->setPos(samplePos);
  sample->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
  instrument->add(sample);
  instrument->markAsSamplePos(sample);

  // A detector
  size_t numdets = vecdetpos.size();
  for (size_t i = 0; i < numdets; ++i) {
    Detector *det =
        new Detector("point-detector", vecdetid[i] /*detector id*/, nullptr);
    det->setPos(vecdetpos[i]);
    // FIXME - should be cubi... pixel
    det->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
    instrument->add(det);
    instrument->markAsDetector(det);
  }

  return instrument;
}

/**
 * Create a sphere object
 */
Object_sptr createSphere(double radius, const V3D &centre,
                         const std::string &id) {
  ShapeFactory shapeMaker;
  return shapeMaker.createShape(sphereXML(radius, centre, id));
}

/**
 * Return the XML for a sphere.
 */
std::string sphereXML(double radius, const Kernel::V3D &centre,
                      const std::string &id) {
  std::ostringstream xml;
  xml << "<sphere id=\"" << id << "\">"
      << "<centre x=\"" << centre.X() << "\"  y=\"" << centre.Y() << "\" z=\""
      << centre.Z() << "\" />"
      << "<radius val=\"" << radius << "\" />"
      << "</sphere>";
  return xml.str();
}

} // namespace ComponentHelper
}
} // namespace Mantid::Geometry
