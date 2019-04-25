// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
namespace ComponentHelper {
using Kernel::V3D;

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
boost::shared_ptr<CSGObject> createSphere(double radius, const V3D &centre,
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
} // namespace Geometry
} // namespace Mantid
