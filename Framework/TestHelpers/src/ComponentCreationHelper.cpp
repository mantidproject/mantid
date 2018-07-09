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
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/make_unique.h"

#include "MantidGeometry/IDetector.h"
#include <Poco/Path.h>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>

using namespace Mantid::Geometry;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

namespace ComponentCreationHelper {
//----------------------------------------------------------------------------------------------

/**
 * Return the XML for a capped cylinder
 */
std::string cappedCylinderXML(double radius, double height,
                              const Mantid::Kernel::V3D &baseCentre,
                              const Mantid::Kernel::V3D &axis,
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
  return xml.str();
}

/**
 * Create a capped cylinder object
 */
boost::shared_ptr<CSGObject> createCappedCylinder(double radius, double height,
                                                  const V3D &baseCentre,
                                                  const V3D &axis,
                                                  const std::string &id) {
  return ShapeFactory().createShape(
      cappedCylinderXML(radius, height, baseCentre, axis, id));
}

void addSourceToInstrument(Instrument_sptr &instrument, const V3D &sourcePos,
                           const std::string &name) {
  ObjComponent *source =
      new ObjComponent(name, IObject_sptr(new CSGObject), instrument.get());
  source->setPos(sourcePos);
  instrument->add(source);
  instrument->markAsSource(source);
}

void addSampleToInstrument(Instrument_sptr &instrument, const V3D &samplePos) {
  // Define a sample as a simple sphere
  IObject_sptr sampleSphere =
      createSphere(0.001, V3D(0.0, 0.0, 0.0), "sample-shape");
  ObjComponent *sample =
      new ObjComponent("sample", sampleSphere, instrument.get());
  instrument->setPos(samplePos);
  instrument->add(sample);
  instrument->markAsSamplePos(sample);
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
boost::shared_ptr<CSGObject> createSphere(double radius, const V3D &centre,
                                          const std::string &id) {
  ShapeFactory shapeMaker;
  return shapeMaker.createShape(sphereXML(radius, centre, id));
}

//----------------------------------------------------------------------------------------------
/** Create a cuboid shape for your pixels */
boost::shared_ptr<CSGObject>
createCuboid(double x_side_length, double y_side_length, double z_side_length) {
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
  auto cuboidShape = shapeCreator.createShape(xmlCuboidShape);
  return cuboidShape;
}

//----------------------------------------------------------------------------------------------
/**
 * Create a component assembly at the origin made up of 4 cylindrical detectors
 */
boost::shared_ptr<CompAssembly> createTestAssemblyOfFourCylinders() {
  boost::shared_ptr<CompAssembly> bank =
      boost::make_shared<CompAssembly>("BankName");
  // One object
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
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
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  return new ObjComponent("pixel", pixelShape);
}

/**
 * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and
 * r2
 */
boost::shared_ptr<CSGObject>
createHollowShell(double innerRadius, double outerRadius, const V3D &centre) {
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
  auto detShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  for (int i = 0; i < ndets; ++i) {
    std::ostringstream os;
    os << "d" << i;
    auto det = boost::make_shared<Detector>(os.str(), i + 1, detShape, nullptr);
    det->setPos(static_cast<double>(i + 1), 2.0, 2.0);
    groupMembers[i] = det;
  }

  return boost::make_shared<DetectorGroup>(groupMembers);
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
  auto detShape = ComponentCreationHelper::createCappedCylinder(
      0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
  for (unsigned int i = 0; i < nDet; ++i) {
    std::ostringstream os;
    os << "d" << i;
    auto det = boost::make_shared<Detector>(os.str(), i + 1, detShape, nullptr);
    det->setPos(double(-0.5 * nDet + i) + gap, 2.0, 2.0);
    groupMembers[i] = det;
  }

  return boost::make_shared<DetectorGroup>(groupMembers);
}

std::vector<std::unique_ptr<IDetector>>
createVectorOfCylindricalDetectors(const double R_min, const double R_max,
                                   const double z0) {
  std::vector<std::unique_ptr<IDetector>> allDetectors;
  // One object
  double R0 = 0.5;
  double h = 1.5;
  auto detShape = ComponentCreationHelper::createCappedCylinder(
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
        auto det = Mantid::Kernel::make_unique<Detector>(os.str(), ic + 1,
                                                         detShape, nullptr);
        det->setPos(x, y, z0);
        allDetectors.emplace_back(std::move(det));
      }

      ic++;
    }
  }
  return allDetectors;
}

//----------------------------------------------------------------------------------------------
/**
 * Create a group of detectors arranged in a ring;
 */
boost::shared_ptr<DetectorGroup>
createRingOfCylindricalDetectors(const double R_min, const double R_max,
                                 const double z0) {

  auto vecOfDetectors = createVectorOfCylindricalDetectors(R_min, R_max, z0);
  std::vector<boost::shared_ptr<const IDetector>> groupMembers;
  groupMembers.reserve(vecOfDetectors.size());
  for (auto &det : vecOfDetectors) {
    groupMembers.push_back(boost::shared_ptr<const IDetector>(std::move(det)));
  }

  return boost::make_shared<DetectorGroup>(groupMembers);
}

Instrument_sptr createTestInstrumentCylindrical(
    int num_banks, const Mantid::Kernel::V3D &sourcePos,
    const Mantid::Kernel::V3D &samplePos, const double cylRadius,
    const double cylHeight) {
  auto testInst = boost::make_shared<Instrument>("basic");

  // One object
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
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
        lexer << "pixel-(" << j << ";" << i << ")";
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

  addSourceToInstrument(testInst, sourcePos);
  addSampleToInstrument(testInst, samplePos);

  return testInst;
}

Mantid::Geometry::Instrument_sptr
createCylInstrumentWithVerticalOffsetsSpecified(
    size_t nTubes, std::vector<double> verticalOffsets, size_t nDetsPerTube,
    double xMin, double xMax, double yMin, double yMax) {
  // Pixel shape
  const double ySpan = (yMax - yMin);
  const double xSpan = (xMax - xMin);
  const double tubeDiameter =
      xSpan / static_cast<double>(nTubes);   // No gaps between tubes
  const double cylRadius = tubeDiameter / 2; // No gaps between tubes
  const double cylHeight = ySpan / static_cast<double>(nDetsPerTube);
  const double bankZPos = 2;
  const double sourceZPos = -10;
  const double sampleZPos = 0;

  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");
  auto instrument = boost::make_shared<Instrument>("instrument_with_tubes");
  CompAssembly *bank = new CompAssembly("sixteenpack");
  for (size_t i = 0; i < nTubes; ++i) {
    ObjCompAssembly *tube = new ObjCompAssembly("tube" + std::to_string(i));
    for (size_t j = 0; j < nDetsPerTube; ++j) {

      auto id = static_cast<int>(i * nDetsPerTube + j);
      Detector *physicalPixel =
          new Detector("det-" + std::to_string(id), id, pixelShape, tube);
      tube->add(physicalPixel);
      physicalPixel->setPos(V3D(0, static_cast<double>(j) * cylHeight, 0));
      instrument->markAsDetector(physicalPixel);
    }
    tube->setPos(V3D(xMin + static_cast<double>(i) * tubeDiameter,
                     -ySpan / 2 + verticalOffsets[i], 0));
    tube->setOutline(tube->createOutline());
    Mantid::Geometry::BoundingBox tmp = tube->shape()->getBoundingBox();
    bank->add(tube);
  }
  bank->setPos(V3D(0, 0, bankZPos));
  instrument->add(bank);
  instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      Mantid::Geometry::Y /*up*/, Mantid::Geometry::Z /*along*/, Left,
      "0,0,0"));
  addSourceToInstrument(instrument, V3D(0, 0, sourceZPos));
  addSampleToInstrument(instrument, V3D(0, 0, sampleZPos));
  return instrument;
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
createCylInstrumentWithDetInGivenPositions(const std::vector<double> &L2,
                                           const std::vector<double> &polar,
                                           const std::vector<double> &azim) {

  auto testInst = boost::make_shared<Instrument>("processed");
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
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");
  // Just increment pixel ID's
  int pixelID = 1;
  // one bank
  CompAssembly *bank = new CompAssembly("det_ass");

  for (size_t i = 0; i < azim.size(); i++) {
    Detector *physicalPixel =
        new Detector("det" + std::to_string(i), pixelID, pixelShape, bank);
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

  addSourceToInstrument(testInst, V3D(0.0, 0.0, -L2_min));
  addSampleToInstrument(testInst, V3D(0.0, 0.0, 0.0));

  return testInst;
}

//----------------------------------------------------------------------------------------------

void addRectangularBank(Instrument &testInstrument, int idStart, int pixels,
                        double pixelSpacing, std::string bankName,
                        const V3D &bankPos, const Quat &bankRot) {

  const double cylRadius(pixelSpacing / 2);
  const double cylHeight(0.0002);
  // One object
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
      cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.),
      "pixel-shape");

  RectangularDetector *bank = new RectangularDetector(bankName);
  bank->initialize(pixelShape, pixels, 0.0, pixelSpacing, pixels, 0.0,
                   pixelSpacing, idStart, true, pixels);

  // Mark them all as detectors
  for (int x = 0; x < pixels; x++)
    for (int y = 0; y < pixels; y++) {
      boost::shared_ptr<Detector> detector = bank->getAtXY(x, y);
      if (detector)
        // Mark it as a detector (add to the instrument cache)
        testInstrument.markAsDetector(detector.get());
    }

  testInstrument.add(bank);
  bank->setPos(bankPos);
  bank->setRot(bankRot);
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
                                                double pixelSpacing,
                                                double bankDistanceFromSample) {
  auto testInst = boost::make_shared<Instrument>("basic_rect");

  for (int banknum = 1; banknum <= num_banks; banknum++) {
    // Make a new bank
    std::ostringstream bankName;
    bankName << "bank" << banknum;
    V3D bankPos(0.0, 0.0, bankDistanceFromSample * banknum);
    Quat bankRot{}; // Identity
    addRectangularBank(*testInst, banknum * pixels * pixels, pixels,
                       pixelSpacing, bankName.str(), bankPos, bankRot);
  }

  addSourceToInstrument(testInst, V3D(0.0, 0.0, -10.0), "source");
  addSampleToInstrument(testInst, V3D(0.0, 0.0, 0.0));

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
  auto testInst = boost::make_shared<Instrument>("basic_rect");

  const double cylRadius(pixelSpacing / 2);
  const double cylHeight(0.0002);
  // One object
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
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

  addSourceToInstrument(testInst, V3D(0.0, 0.0, -10.0));
  addSampleToInstrument(testInst, V3D(0.0, 0.0, 0.0));

  return testInst;
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
Instrument_sptr
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

CompAssembly *makeBank(size_t width, size_t height, Instrument *instrument) {

  double width_d = double(width);
  double height_d = double(height);
  static int bankNo = 1;
  auto bank = new CompAssembly("Bank" + std::to_string(bankNo++));
  static size_t id = 1;
  for (size_t i = 0; i < width; ++i) {
    for (size_t j = 0; j < height; ++j) {
      Detector *det = new Detector("pixel", int(id++) /*detector id*/, bank);
      det->setPos(V3D{double(i), double(j), double(0)});
      det->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), "1"));
      bank->add(det);
      instrument->markAsDetector(det);
    }
  }
  bank->setPos(V3D{width_d / 2, height_d / 2, 0});

  return bank;
}

Instrument_sptr sansInstrument(const Mantid::Kernel::V3D &sourcePos,
                               const Mantid::Kernel::V3D &samplePos,
                               const Mantid::Kernel::V3D &trolley1Pos,
                               const Mantid::Kernel::V3D &trolley2Pos) {

  /*
  This has been generated for comparison with newer Instrument designs. It is
  therefore not
  an exact representation of an instrument one might expect to create for SANS.
   */
  auto instrument = boost::make_shared<Instrument>();

  instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      Mantid::Geometry::Y /*up*/, Mantid::Geometry::Z /*along*/, Left,
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

  size_t width = 100;
  size_t height = 100;

  CompAssembly *trolley1 = new CompAssembly("Trolley1");
  trolley1->setPos(trolley1Pos);
  CompAssembly *trolley2 = new CompAssembly("Trolley2");
  trolley2->setPos(trolley2Pos);

  CompAssembly *N = makeBank(width, height, instrument.get());
  trolley1->add(N);
  CompAssembly *E = makeBank(width, height, instrument.get());
  trolley1->add(E);
  CompAssembly *S = makeBank(width, height, instrument.get());
  trolley1->add(S);
  CompAssembly *W = makeBank(width, height, instrument.get());
  trolley1->add(W);

  CompAssembly *l_curtain = makeBank(width, height, instrument.get());
  trolley2->add(l_curtain);
  CompAssembly *r_curtain = makeBank(width, height, instrument.get());
  trolley2->add(r_curtain);

  instrument->add(trolley1);
  instrument->add(trolley2);
  return instrument;
}

Mantid::Geometry::Instrument_sptr
createInstrumentWithPSDTubes(const size_t nTubes, const size_t nPixelsPerTube,
                             bool mirrorTubes) {
  // Need a tube based instrument.
  //
  // Pixels will be numbered simply from 1->nTubes*nPixelsPerTube with a 1:1
  // mapping
  //
  // Tubes will be located at 1 m from the sample (0, 0, 0) from 0 -> 90 deg
  // If mirror is set to true they will go from 0 -> -90 deg
  Instrument_sptr testInst(new Instrument("PSDTubeInst"));
  int xDirection(1);
  if (mirrorTubes)
    xDirection = -1;

  testInst->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      Mantid::Geometry::Y, Mantid::Geometry::Z, Mantid::Geometry::X, Right,
      "0,0,0"));

  // Pixel shape
  const double pixelRadius(0.01);
  const double pixelHeight(0.003);
  const double radius(1.0);
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
      pixelRadius, pixelHeight, V3D(0.0, -0.5 * pixelHeight, 0.0),
      V3D(0.0, 1.0, 0.0), "pixelShape");
  for (size_t i = 0; i < nTubes; ++i) {
    std::ostringstream lexer;
    lexer << "tube-" << i;
    const auto theta = (M_PI / 2.0) * double(i) / (double(nTubes) - 1);
    auto x = xDirection * radius * sin(theta);
    // A small correction to make testing easier where the instrument is
    // mirrored
    if (i == 0 && xDirection < 0)
      x = -1e-32;
    const auto z = radius * cos(theta);
    CompAssembly *tube = new CompAssembly(lexer.str());
    tube->setPos(V3D(x, 0.0, z));
    for (size_t j = 0; j < nPixelsPerTube; ++j) {
      lexer.str("");
      lexer << "pixel-" << i * nPixelsPerTube + j;
      Detector *pixel = new Detector(
          lexer.str(), int(i * nPixelsPerTube + j + 1), pixelShape, tube);
      const double xpos = 0.0;
      const double ypos = double(j) * pixelHeight;
      pixel->setPos(xpos, ypos, 0.0);
      tube->add(pixel);
      testInst->markAsDetector(pixel);
    }
    testInst->add(tube);
  }
  addSourceToInstrument(testInst, V3D(0.0, 0.0, -1.0));
  addSampleToInstrument(testInst, V3D(0.0, 0.0, 0.0));

  return testInst;
}
} // namespace ComponentCreationHelper
