/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY NOT be used in any test from a package
 *below
 *  Geometry (e.g. Kernel).
 *  Conversely, this file MAY NOT be modified to use anything from a package
 *  higher than Geometry (e.g. API, DataObjects, ...)
 *********************************************************************************/
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <Poco/Path.h>
#include <boost/shared_array.hpp>
#include <boost/make_shared.hpp>
#include "MantidGeometry/IDetector.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Quat;

namespace ComponentCreationHelper {
//----------------------------------------------------------------------------------------------
/**
 * Create a capped cylinder object
 */
Object_sptr createCappedCylinder(double radius, double height,
                                 const V3D &baseCentre, const V3D &axis,
                                 const std::string &id) {
  std::ostringstream xml;
  xml << "<cylinder id=\"" << id << "\">"
      << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\""
      << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
      << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\""
      << axis.Z() << "\"/>"
      << "<radius val=\"" << radius << "\" />"
      << "<height val=\"" << height << "\" />"
      << "</cylinder>";

  ShapeFactory shapeMaker;
  return shapeMaker.createShape(xml.str());
}

//----------------------------------------------------------------------------------------------

/**
 * Return the XML for a sphere.
 */
std::string sphereXML(double radius, const V3D &centre, const std::string &id) {
  std::ostringstream xml;
  xml << "<sphere id=\"" << id << "\">"
      << "<centre x=\"" << centre.X() << "\"  y=\"" << centre.Y() << "\" z=\""
      << centre.Z() << "\" />"
      << "<radius val=\"" << radius << "\" />"
      << "</sphere>";
  return xml.str();
}

/**
 * Create a sphere object
 */
Object_sptr createSphere(double radius, const V3D &centre,
                         const std::string &id) {
  ShapeFactory shapeMaker;
  return shapeMaker.createShape(sphereXML(radius, centre, id));
}

//----------------------------------------------------------------------------------------------
/** Create a cuboid shape for your pixels */
Object_sptr createCuboid(double x_side_length, double y_side_length,
                         double z_side_length) {
  double szX = x_side_length;
  double szY = (y_side_length == -1.0 ? szX : y_side_length);
  double szZ = (z_side_length == -1.0 ? szX : z_side_length);
  std::ostringstream xmlShapeStream;
  xmlShapeStream << " <cuboid id=\"detector-shape\"> "
                 << "<left-front-bottom-point x=\"" << szX << "\" y=\"" << -szY
                 << "\" z=\"" << -szZ << "\"  /> "
                 << "<left-front-top-point  x=\"" << szX << "\" y=\"" << -szY
                 << "\" z=\"" << szZ << "\"  /> "
                 << "<left-back-bottom-point  x=\"" << -szX << "\" y=\"" << -szY
                 << "\" z=\"" << -szZ << "\"  /> "
                 << "<right-front-bottom-point  x=\"" << szX << "\" y=\"" << szY
                 << "\" z=\"" << -szZ << "\"  /> "
                 << "</cuboid>";

  std::string xmlCuboidShape(xmlShapeStream.str());
  ShapeFactory shapeCreator;
  Object_sptr cuboidShape = shapeCreator.createShape(xmlCuboidShape);
  return cuboidShape;
}

//----------------------------------------------------------------------------------------------
/**
* Create a component assembly at the origin made up of 4 cylindrical detectors
*/
boost::shared_ptr<CompAssembly> createTestAssemblyOfFourCylinders() {
  boost::shared_ptr<CompAssembly> bank =
      boost::shared_ptr<CompAssembly>(new CompAssembly("BankName"));
  // One object
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  // Four object components
  for (size_t i = 1; i < 5; ++i) {
    ObjComponent *physicalPixel = new ObjComponent("pixel", pixelShape);
    physicalPixel->setPos(static_cast<double>(i), 0.0, 0.0);
    bank->add(physicalPixel);
  }

  return bank;
}

/**
 * Create an object component that has a defined shape
 */
ObjComponent *createSingleObjectComponent() {
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  return new ObjComponent("pixel", pixelShape);
}

/**
 * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and
 * r2
 */
Object_sptr createHollowShell(double innerRadius, double outerRadius,
                              const V3D &centre) {
  std::string wholeXML = sphereXML(innerRadius, centre, "inner") + "\n" +
                         sphereXML(outerRadius, centre, "outer") + "\n" +
                         "<algebra val=\"(outer (# inner))\" />";

  ShapeFactory shapeMaker;
  return shapeMaker.createShape(wholeXML);
}

//----------------------------------------------------------------------------------------------
/**
 * Create a detector group containing 5 detectors
 */
boost::shared_ptr<DetectorGroup>
createDetectorGroupWith5CylindricalDetectors() {
  const int ndets = 5;
  std::vector<boost::shared_ptr<const IDetector>> groupMembers(ndets);
  // One object
  Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  for (int i = 0; i < ndets; ++i) {
    std::ostringstream os;
    os << "d" << i;
    boost::shared_ptr<Detector> det(
        new Detector(os.str(), i + 1, detShape, NULL));
    det->setPos((double)(i + 1), 2.0, 2.0);
    groupMembers[i] = det;
  }

  return boost::shared_ptr<DetectorGroup>(
      new DetectorGroup(groupMembers, false));
}

//----------------------------------------------------------------------------------------------
/**
 * Create a detector group containing N cylindrical detectors with gaps
 */
boost::shared_ptr<DetectorGroup>
createDetectorGroupWithNCylindricalDetectorsWithGaps(unsigned int nDet,
                                                     double gap) {

  std::vector<boost::shared_ptr<const IDetector>> groupMembers(nDet);
  // One object
  Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  for (unsigned int i = 0; i < nDet; ++i) {
    std::ostringstream os;
    os << "d" << i;
    boost::shared_ptr<Detector> det(
        new Detector(os.str(), i + 1, detShape, NULL));
    det->setPos(double(-0.5 * nDet + i) + gap, 2.0, 2.0);
    groupMembers[i] = det;
  }

  return boost::shared_ptr<DetectorGroup>(
      new DetectorGroup(groupMembers, false));
}

//----------------------------------------------------------------------------------------------
/**
 * Create a group of detectors arranged in a ring;
 */
boost::shared_ptr<DetectorGroup>
createRingOfCylindricalDetectors(const double R_min, const double R_max,
                                 const double z0) {

  std::vector<boost::shared_ptr<const IDetector>> groupMembers;
  // One object
  double R0 = 0.5;
  double h = 1.5;
  Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(
      R0, h, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

  int NY = int(ceil(2 * R_max / h) + 1);
  int NX = int(ceil(2 * R_max / R0) + 1);
  double y_bl = NY * h;
  double x_bl = NX * R0;

  double Rmin2(R_min * R_min), Rmax2(R_max * R_max);

  int ic(0);
  for (int j = 0; j < NY; j++) {
    double y = -0.5 * y_bl + j * h;
    for (int i = 0; i < NX; i++) {
      double x = -0.5 * x_bl + i * R0;
      double Rsq = x * x + y * y;
      if (Rsq >= Rmin2 && Rsq < Rmax2) {
        std::ostringstream os;
        os << "d" << ic;
        boost::shared_ptr<Detector> det(
            new Detector(os.str(), ic + 1, detShape, NULL));
        det->setPos(x, y, z0);
        groupMembers.push_back(det);
      }

      ic++;
    }
  }
  return boost::shared_ptr<DetectorGroup>(
      new DetectorGroup(groupMembers, false));
}

//----------------------------------------------------------------------------------------------
/**
 * Create a group of two monitors
 */
boost::shared_ptr<DetectorGroup> createGroupOfTwoMonitors() {
  const int ndets(2);
  std::vector<boost::shared_ptr<const IDetector>> groupMembers(ndets);
  for (int i = 0; i < ndets; ++i) {
    std::ostringstream os;
    os << "m" << i;
    boost::shared_ptr<Detector> det(new Detector(os.str(), i + 1, NULL));
    det->setPos((double)(i + 1), 2.0, 2.0);
    det->markAsMonitor();
    groupMembers[i] = det;
  }
  return boost::shared_ptr<DetectorGroup>(
      new DetectorGroup(groupMembers, false));
}

//----------------------------------------------------------------------------------------------

Instrument_sptr createTestInstrumentCylindrical(int num_banks, bool verbose,
                                                const double cylRadius,
                                                const double cylHeight) {
  boost::shared_ptr<Instrument> testInst(new Instrument("basic"));

  // One object
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");

  // Just increment pixel IDs
  int pixelID = 1;

  for (int banknum = 1; banknum <= num_banks; banknum++) {
    // Make a new bank
    std::ostringstream bankname;
    bankname << "bank" << banknum;
    CompAssembly *bank = new CompAssembly(bankname.str());

    // Nine object components
    for (int i = -1; i < 2; ++i) {
      for (int j = -1; j < 2; ++j) {
        std::ostringstream lexer;
        lexer << "pixel-(" << j << "," << i << ")";
        Detector *physicalPixel =
            new Detector(lexer.str(), pixelID, pixelShape, bank);
        const double xpos = j * (cylRadius * 2.0);
        const double ypos = i * cylHeight;
        physicalPixel->setPos(xpos, ypos, 0.0);
        pixelID++;
        bank->add(physicalPixel);
        testInst->markAsDetector(physicalPixel);
      }
    }

    testInst->add(bank);
    bank->setPos(V3D(0.0, 0.0, 5.0 * banknum));
  }

  // Define a source component
  ObjComponent *source =
      new ObjComponent("moderator", Object_sptr(new Object), testInst.get());
  source->setPos(V3D(0.0, 0.0, -10.));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  Object_sptr sampleSphere =
      createSphere(0.001, V3D(0.0, 0.0, 0.0), "sample-shape");
  ObjComponent *sample =
      new ObjComponent("sample", sampleSphere, testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  if (verbose) {
    std::cout << "\n\n=== Testing bank positions ==\n";
    const int nchilds = testInst->nelements();
    for (int i = 0; i < nchilds; ++i) {
      boost::shared_ptr<IComponent> child = testInst->getChild(i);
      std::cout << "Component " << i << " at pos " << child->getPos() << "\n";
      if (boost::shared_ptr<ICompAssembly> assem =
              boost::dynamic_pointer_cast<ICompAssembly>(child)) {
        for (int j = 0; j < assem->nelements(); ++j) {
          boost::shared_ptr<IComponent> comp = assem->getChild(j);
          std::cout << "Child " << j << " at pos " << comp->getPos() << "\n";
        }
      }
    }
    std::cout << "==================================\n";
  }

  return testInst;
}

/** create instrument with cylindrical detecotrs located in specific positions
*
*
*/
bool double_cmprsn(double x1, double x2) {
  const double TOL(1.e-4);
  if (std::fabs(x1 + x2) < TOL) {
    return (std::fabs(x1 - x2) < TOL);
  } else {
    return (std::fabs((x1 - x2) / (x1 + x2)) < TOL / 2);
  }
}
Mantid::Geometry::Instrument_sptr
createCylInstrumentWithDetInGivenPosisions(const std::vector<double> &L2,
                                           const std::vector<double> &polar,
                                           const std::vector<double> &azim) {
  boost::shared_ptr<Instrument> testInst(new Instrument("processed"));

  double cylRadius(0.004);
  double cylHeight(0.0002);
  // find characteristic sizes of the detectors;
  double dAzi_min(FLT_MAX);
  double dPol_min(FLT_MAX);
  double L2_min(FLT_MAX);
  double dAzi, dPol;
  std::vector<double> az(azim);
  std::vector<double> po(polar);
  std::sort(az.begin(), az.end());
  std::sort(po.begin(), po.end());
  // very crude identification of interdetector distance; no need in more
  // accurate caluclations for example;
  for (size_t i = 0; i < L2.size(); i++) {
    if (L2[i] < L2_min)
      L2_min = L2[i];
    for (size_t j = i + 1; j < L2.size(); j++) {
      if (!double_cmprsn(az[i], az[j])) {
        dAzi = std::fabs(az[i] - az[j]);
        if (dAzi < dAzi_min)
          dAzi_min = dAzi;
      }
      if (!double_cmprsn(po[i], po[j])) {
        dPol = std::fabs(po[i] - po[j]);
        if (dPol < dPol_min)
          dPol_min = dPol;
      }
    }
  }
  cylRadius = L2_min * sin(dAzi_min * 0.5);
  cylHeight = 2 * L2_min * sin(dPol_min * 0.5);

  // One object
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");
  // Just increment pixel ID's
  int pixelID = 1;
  // one bank
  CompAssembly *bank = new CompAssembly("det_ass");

  for (size_t i = 0; i < azim.size(); i++) {
    Detector *physicalPixel = new Detector(
        "det" + boost::lexical_cast<std::string>(i), pixelID, pixelShape, bank);
    double zpos = L2[i] * cos(polar[i]);
    double xpos = L2[i] * sin(polar[i]) * cos(azim[i]);
    double ypos = L2[i] * sin(polar[i]) * sin(azim[i]);
    physicalPixel->setPos(xpos, ypos, zpos);
    pixelID++;
    bank->add(physicalPixel);
    testInst->markAsDetector(physicalPixel);
  }
  testInst->add(bank);
  bank->setPos(V3D(0., 0., 0.));

  // Define a source component
  ObjComponent *source =
      new ObjComponent("moderator", Object_sptr(new Object), testInst.get());
  source->setPos(V3D(0.0, 0.0, -L2_min));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  Object_sptr sampleSphere =
      createSphere(cylRadius, V3D(0.0, 0.0, 0.0), "sample-shape");
  ObjComponent *sample =
      new ObjComponent("sample", sampleSphere, testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  return testInst;
}

//----------------------------------------------------------------------------------------------
/**
 * Create an test instrument with n panels of rectangular detectors,
 *pixels*pixels in size,
 * a source and spherical sample shape.
 *
 * Banks' lower-left corner is at position (0,0,5*banknum) and they go up to
 *(pixels*0.008, pixels*0.008, Z)
 * Pixels are 4 mm wide.
 *
 * @param num_banks :: number of rectangular banks to create
 * @param pixels :: number of pixels in each direction.
 * @param pixelSpacing :: padding between pixels
 * @param bankDistanceFromSample :: How far the bank is from the sample
 */
Instrument_sptr createTestInstrumentRectangular(int num_banks, int pixels,
                                                double pixelSpacing, double bankDistanceFromSample) {
  boost::shared_ptr<Instrument> testInst(new Instrument("basic_rect"));

  const double cylRadius(pixelSpacing / 2);
  const double cylHeight(0.0002);
  // One object
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");

  for (int banknum = 1; banknum <= num_banks; banknum++) {
    // Make a new bank
    std::ostringstream bankname;
    bankname << "bank" << banknum;

    RectangularDetector *bank = new RectangularDetector(bankname.str());
    bank->initialize(pixelShape, pixels, 0.0, pixelSpacing, pixels, 0.0,
                     pixelSpacing, banknum * pixels * pixels, true, pixels);

    // Mark them all as detectors
    for (int x = 0; x < pixels; x++)
      for (int y = 0; y < pixels; y++) {
        boost::shared_ptr<Detector> detector = bank->getAtXY(x, y);
        if (detector)
          // Mark it as a detector (add to the instrument cache)
          testInst->markAsDetector(detector.get());
      }

    testInst->add(bank);
    bank->setPos(V3D(0.0, 0.0, bankDistanceFromSample * banknum));
  }

  // Define a source component
  ObjComponent *source =
      new ObjComponent("source", createSphere(0.01 /*1cm*/, V3D(0,0,0), "1"), testInst.get());
  source->setPos(V3D(0.0, 0.0, -10.));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  Object_sptr sampleSphere =
      createSphere(0.001, V3D(0.0, 0.0, 0.0), "sample-shape");
  ObjComponent *sample =
      new ObjComponent("sample", sampleSphere, testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  return testInst;
}

//----------------------------------------------------------------------------------------------
/**
 * Create an test instrument with n panels of rectangular detectors,
 *pixels*pixels in size,
 * a source and spherical sample shape.
 *
 * Banks are centered at (1*banknum, 0, 0) and are facing 0,0.
 * Pixels are 4 mm wide.
 *
 * @param num_banks: number of rectangular banks to create
 * @param pixels :: number of pixels in each direction.
 * @param pixelSpacing :: padding between pixels
 */
Instrument_sptr createTestInstrumentRectangular2(int num_banks, int pixels,
                                                 double pixelSpacing) {
  boost::shared_ptr<Instrument> testInst(new Instrument("basic_rect"));

  const double cylRadius(pixelSpacing / 2);
  const double cylHeight(0.0002);
  // One object
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");

  for (int banknum = 1; banknum <= num_banks; banknum++) {
    // Make a new bank
    std::ostringstream bankname;
    bankname << "bank" << banknum;

    RectangularDetector *bank = new RectangularDetector(bankname.str());
    bank->initialize(pixelShape, pixels, -pixels * pixelSpacing / 2.0,
                     pixelSpacing, pixels, -pixels * pixelSpacing / 2.0,
                     pixelSpacing, (banknum - 1) * pixels * pixels, true,
                     pixels);

    // Mark them all as detectors
    for (int x = 0; x < pixels; x++)
      for (int y = 0; y < pixels; y++) {
        boost::shared_ptr<Detector> detector = bank->getAtXY(x, y);
        if (detector)
          // Mark it as a detector (add to the instrument cache)
          testInst->markAsDetector(detector.get());
      }

    testInst->add(bank);
    // Place the center.
    bank->setPos(V3D(1.0 * banknum, 0.0, 0.0));
    // rotate detector 90 degrees along vertical
    bank->setRot(Quat(90.0, V3D(0, 1, 0)));
  }

  // Define a source component
  ObjComponent *source =
      new ObjComponent("moderator", Object_sptr(new Object), testInst.get());
  source->setPos(V3D(0.0, 0.0, -10.));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  Object_sptr sampleSphere =
      createSphere(0.001, V3D(0.0, 0.0, 0.0), "sample-shape");
  ObjComponent *sample =
      new ObjComponent("sample", sampleSphere, testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  return testInst;
}

/**
 * createOneDetectorInstrument, creates the most simple possible definition of an instrument in which we can extract a valid L1 and L2 distance for unit calculations.
 *
 * Beam direction is along X,
 * Up direction is Y
 *
 * @param sourcePos : V3D position
 * @param samplePos : V3D sample position
 * @param detectorPos : V3D detector position
 * @return Instrument generated.
 */
Instrument_sptr createMinimalInstrument(const Mantid::Kernel::V3D& sourcePos, const Mantid::Kernel::V3D& samplePos, const Mantid::Kernel::V3D& detectorPos )
{
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Mantid::Geometry::Y /*up*/, Mantid::Geometry::X /*along*/, Left, "0,0,0"));

    // A source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(sourcePos);
    source->setShape(createSphere(0.01 /*1cm*/, V3D(0,0,0), "1"));
    instrument->add(source);
    instrument->markAsSource(source);

    // A sample
    ObjComponent *sample = new ObjComponent("some-surface-holder");
    sample->setPos(samplePos);
    sample->setShape(createSphere(0.01 /*1cm*/, V3D(0,0,0), "1"));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    // A detector
    Detector *det = new Detector("point-detector", 1 /*detector id*/, NULL);
    det->setPos(detectorPos);
    det->setShape(createSphere(0.01 /*1cm*/, V3D(0,0,0), "1"));
    instrument->add(det);
    instrument->markAsDetector(det);

    return instrument;
}
}
