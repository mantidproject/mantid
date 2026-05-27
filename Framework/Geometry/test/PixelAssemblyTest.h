// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/PixelAssembly.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class PixelAssemblyTest : public CxxTest::TestSuite {
public:
  // ---- helpers -------------------------------------------------------------

  /// Build a 4 × 5 × 3 assembly at the origin with X-fastest fill order.
  static PixelAssembly *makeAssembly() {
    auto *det = new PixelAssembly("MyBank");
    det->setPos(0., 0., 0.);
    det->initialize(ComponentCreationHelper::createCuboid(0.5), 4, -2.0, 1.0, // xpixels, xstart, xstep
                    5, -2.5, 1.0,                                             // ypixels, ystart, ystep
                    3, 0.0, 1.0,                                              // zpixels, zstart, zstep
                    1000, "xyz", 1);
    return det;
  }

  // ---- constructor / name --------------------------------------------------

  void testDefaultConstructor() {
    PixelAssembly pa("TestAssembly");
    TS_ASSERT_EQUALS(pa.getName(), "TestAssembly");
    TS_ASSERT(!pa.getParent());
    TS_ASSERT_EQUALS(pa.type(), "PixelAssembly");
  }

  void testCompareName() {
    TS_ASSERT(PixelAssembly::compareName("PixelAssembly"));
    TS_ASSERT(PixelAssembly::compareName("pixelassembly"));
    TS_ASSERT(PixelAssembly::compareName("pixel_assembly"));
    TS_ASSERT(PixelAssembly::compareName("PIXEL_ASSEMBLY"));
    TS_ASSERT(!PixelAssembly::compareName("GridDetector"));
    TS_ASSERT(!PixelAssembly::compareName("Pixel"));
  }

  // ---- grid accessors ------------------------------------------------------

  void testGridAccessors() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    TS_ASSERT_EQUALS(det->xpixels(), 4u);
    TS_ASSERT_EQUALS(det->ypixels(), 5u);
    TS_ASSERT_EQUALS(det->zpixels(), 3u);
    TS_ASSERT_DELTA(det->xstart(), -2.0, 1e-10);
    TS_ASSERT_DELTA(det->ystart(), -2.5, 1e-10);
    TS_ASSERT_DELTA(det->zstart(), 0.0, 1e-10);
    TS_ASSERT_DELTA(det->xstep(), 1.0, 1e-10);
    TS_ASSERT_DELTA(det->ystep(), 1.0, 1e-10);
    TS_ASSERT_DELTA(det->zstep(), 1.0, 1e-10);
    TS_ASSERT_DELTA(det->xsize(), 4.0, 1e-10);
    TS_ASSERT_DELTA(det->ysize(), 5.0, 1e-10);
    TS_ASSERT_DELTA(det->zsize(), 3.0, 1e-10);
    TS_ASSERT_EQUALS(det->npixels(), 4u * 5u * 3u);
  }

  // ---- ID scheme -----------------------------------------------------------

  void testIDScheme() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    TS_ASSERT_EQUALS(det->referentDetectorID(), 1000);
    TS_ASSERT_EQUALS(det->idstep(), 1);
    auto order = det->idFillOrder();
    TS_ASSERT_EQUALS(order[0], 'x');
    TS_ASSERT_EQUALS(order[1], 'y');
    TS_ASSERT_EQUALS(order[2], 'z');
  }

  void testMinMaxDetectorID() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    // fill order xyz, step=1, nx=4, ny=5, nz=3
    // min = 1000 (referent)
    // max at (3,4,2) = 1000 + 3 + 4*4 + 2*20 = 1000+3+16+40 = 1059
    TS_ASSERT_EQUALS(det->minDetectorID(), 1000);
    TS_ASSERT_EQUALS(det->maxDetectorID(), 1059);
  }

  void testGetDetectorIDAtXYZ() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(0, 0, 0), 1000);
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(1, 0, 0), 1001); // X step=1
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(0, 1, 0), 1004); // Y stride=nx=4
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(0, 0, 1), 1020); // Z stride=nx*ny=20
    TS_ASSERT_EQUALS(det->getDetectorIDAtXYZ(2, 3, 1), 1000 + 2 + 3 * 4 + 1 * 20);
  }

  void testGetXYZForDetectorID() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    auto [x, y, z] = det->getXYZForDetectorID(1000);
    TS_ASSERT_EQUALS(x, 0);
    TS_ASSERT_EQUALS(y, 0);
    TS_ASSERT_EQUALS(z, 0);

    auto [x2, y2, z2] = det->getXYZForDetectorID(1059);
    TS_ASSERT_EQUALS(x2, 3);
    TS_ASSERT_EQUALS(y2, 4);
    TS_ASSERT_EQUALS(z2, 2);
  }

  // ---- bounds check --------------------------------------------------------

  void testInBoundsXYZ() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly()); // 4×5×3
    TS_ASSERT(det->inBoundsXYZ(0, 0, 0));
    TS_ASSERT(det->inBoundsXYZ(3, 4, 2));
    TS_ASSERT(!det->inBoundsXYZ(-1, 0, 0));
    TS_ASSERT(!det->inBoundsXYZ(4, 0, 0)); // == xpixels
    TS_ASSERT(!det->inBoundsXYZ(0, 5, 0)); // == ypixels
    TS_ASSERT(!det->inBoundsXYZ(0, 0, 3)); // == zpixels
  }

  // ---- position helpers ----------------------------------------------------

  void testGetRelativePosAtXYZ() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    V3D pos = det->getRelativePosAtXYZ(0, 0, 0);
    TS_ASSERT_DELTA(pos.X(), -2.0, 1e-10);
    TS_ASSERT_DELTA(pos.Y(), -2.5, 1e-10);
    TS_ASSERT_DELTA(pos.Z(), 0.0, 1e-10);

    V3D pos2 = det->getRelativePosAtXYZ(3, 4, 2);
    TS_ASSERT_DELTA(pos2.X(), -2.0 + 3.0, 1e-10);
    TS_ASSERT_DELTA(pos2.Y(), -2.5 + 4.0, 1e-10);
    TS_ASSERT_DELTA(pos2.Z(), 0.0 + 2.0, 1e-10);
  }

  void testGetPosAtXYZ() {
    auto *det = makeAssembly();
    det->setPos(10., 20., 30.);
    std::unique_ptr<PixelAssembly> u(det);

    // Bank at (10,20,30), referent at local (-2,-2.5,0) → world (8, 17.5, 30)
    V3D pos = det->getPosAtXYZ(0, 0, 0);
    TS_ASSERT_DELTA(pos.X(), 10. - 2., 1e-10);
    TS_ASSERT_DELTA(pos.Y(), 20. - 2.5, 1e-10);
    TS_ASSERT_DELTA(pos.Z(), 30., 1e-10);

    // Pixel (3,4,2): local (1, 1.5, 2) → world (11, 21.5, 32)
    V3D pos2 = det->getPosAtXYZ(3, 4, 2);
    TS_ASSERT_DELTA(pos2.X(), 10. + 1., 1e-10);
    TS_ASSERT_DELTA(pos2.Y(), 20. + 1.5, 1e-10);
    TS_ASSERT_DELTA(pos2.Z(), 30. + 2., 1e-10);
  }

  // ---- flat bank (zpixels = 0 normalised to 1) ----------------------------

  void testFlatBank() {
    auto *det = new PixelAssembly("FlatBank");
    det->setPos(0., 0., 0.);
    // Pass zpixels=0 → should be normalised to 1.
    det->initialize(ComponentCreationHelper::createCuboid(0.5), 3, 0.0, 1.0, // xpixels=3
                    4, 0.0, 1.0,                                             // ypixels=4
                    0, 0.0, 0.0,                                             // zpixels=0 → flat
                    0, "xyz", 1);
    std::unique_ptr<PixelAssembly> u(det);
    TS_ASSERT_EQUALS(det->zpixels(), 1u); // normalised
    TS_ASSERT_EQUALS(det->npixels(), 3u * 4u * 1u);
  }

  // ---- parametrized version -----------------------------------------------

  void testParametrizedScaling() {
    auto *det = makeAssembly();
    std::unique_ptr<PixelAssembly> u(det);
    ParameterMap_sptr pmap(new ParameterMap());
    PixelAssembly parDet(det, pmap.get());

    pmap->addDouble(det, "scalex", 2.0);
    pmap->addDouble(det, "scaley", 3.0);
    pmap->addDouble(det, "scalez", 4.0);

    TS_ASSERT_DELTA(parDet.xstep(), 2.0, 1e-10);
    TS_ASSERT_DELTA(parDet.ystep(), 3.0, 1e-10);
    TS_ASSERT_DELTA(parDet.zstep(), 4.0, 1e-10);
    TS_ASSERT_DELTA(parDet.xstart(), -4.0, 1e-10); // -2.0 * 2.0
    TS_ASSERT_DELTA(parDet.ystart(), -7.5, 1e-10); // -2.5 * 3.0
    TS_ASSERT_DELTA(parDet.zstart(), 0.0, 1e-10);
    TS_ASSERT_DELTA(parDet.xsize(), 8.0, 1e-10);  // 4 * 2.0
    TS_ASSERT_DELTA(parDet.ysize(), 15.0, 1e-10); // 5 * 3.0
    TS_ASSERT_DELTA(parDet.zsize(), 12.0, 1e-10); // 3 * 4.0

    // Pixel counts are unchanged by scale factors.
    TS_ASSERT_EQUALS(parDet.xpixels(), 4u);
    TS_ASSERT_EQUALS(parDet.ypixels(), 5u);
    TS_ASSERT_EQUALS(parDet.zpixels(), 3u);

    // ID scheme unchanged.
    TS_ASSERT_EQUALS(parDet.referentDetectorID(), 1000);
  }

  // ---- bounding box --------------------------------------------------------

  void testBoundingBoxAtXYZ() {
    auto *det = makeAssembly(); // 4×5×3, 1×1×1 steps, cuboid 0.5 half-size
    det->setPos(0., 0., 0.);
    std::unique_ptr<PixelAssembly> u(det);

    BoundingBox box;
    det->getBoundingBoxAtXYZ(0, 0, 0, box);
    // Referent pixel at local (-2, -2.5, 0); cuboid half-size 0.5
    TS_ASSERT_DELTA(box.xMin(), -2.5, 1e-8);
    TS_ASSERT_DELTA(box.xMax(), -1.5, 1e-8);
    TS_ASSERT_DELTA(box.yMin(), -3.0, 1e-8);
    TS_ASSERT_DELTA(box.yMax(), -2.0, 1e-8);
    TS_ASSERT_DELTA(box.zMin(), -0.5, 1e-8);
    TS_ASSERT_DELTA(box.zMax(), 0.5, 1e-8);
  }

  void testGetBoundingBox() {
    auto *det = makeAssembly(); // bank at origin, 4×5×3 pixels, step=1
    det->setPos(0., 0., 0.);
    std::unique_ptr<PixelAssembly> u(det);

    BoundingBox box;
    det->getBoundingBox(box);
    // x: first pixel at -2 ± 0.5 → xMin=-2.5; last at (-2+3*1)=1 ± 0.5 → xMax=1.5
    TS_ASSERT_DELTA(box.xMin(), -2.5, 1e-8);
    TS_ASSERT_DELTA(box.xMax(), 1.5, 1e-8);
    // y: first at -2.5 → yMin=-3.0; last at (-2.5+4*1)=1.5 → yMax=2.0
    TS_ASSERT_DELTA(box.yMin(), -3.0, 1e-8);
    TS_ASSERT_DELTA(box.yMax(), 2.0, 1e-8);
    // z: first at 0 → zMin=-0.5; last at (0+2*1)=2 → zMax=2.5
    TS_ASSERT_DELTA(box.zMin(), -0.5, 1e-8);
    TS_ASSERT_DELTA(box.zMax(), 2.5, 1e-8);
  }

  // ---- no children ---------------------------------------------------------

  void testNoChildrenExist() {
    // PixelAssembly must not inherit from ICompAssembly — verify it has no
    // children by confirming the cast fails.
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    ICompAssembly const *asAssembly = dynamic_cast<ICompAssembly const *>(det.get());
    TS_ASSERT_EQUALS(asAssembly, nullptr);
  }

  // ---- pixelShape ----------------------------------------------------------

  void testPixelShape() {
    auto det = std::unique_ptr<PixelAssembly>(makeAssembly());
    auto ps = det->pixelShape();
    TS_ASSERT(ps);
    TS_ASSERT(ps->hasValidShape());
  }
};
