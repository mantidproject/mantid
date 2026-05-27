// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument/IVirtualBank.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid;
using namespace Mantid::Geometry;

namespace {

/**
 * Minimal concrete subclass used to exercise the IVirtualBank base-class
 * methods.  The grid is x-major (X varies fastest), then Y, then Z, which
 * corresponds to idFillOrder = {'x','y','z'}.
 */
class TestBank : public IVirtualBank {
public:
  TestBank(size_t nx, size_t ny, size_t nz, double xstrt, double ystrt, double zstrt, double dxstep, double dystep,
           double dzstep, detid_t idstart, std::array<char, 3> fillOrder, int step)
      : m_nx(nx), m_ny(ny), m_nz(nz), m_xstart(xstrt), m_ystart(ystrt), m_zstart(zstrt), m_xstep(dxstep),
        m_ystep(dystep), m_zstep(dzstep), m_idstart(idstart), m_fillOrder(fillOrder), m_idstep(step) {}

  size_t xpixels() const override { return m_nx; }
  size_t ypixels() const override { return m_ny; }
  size_t zpixels() const override { return m_nz; }
  double xstep() const override { return m_xstep; }
  double ystep() const override { return m_ystep; }
  double zstep() const override { return m_zstep; }
  double xstart() const override { return m_xstart; }
  double ystart() const override { return m_ystart; }
  double zstart() const override { return m_zstart; }
  detid_t referentDetectorID() const override { return m_idstart; }
  std::array<char, 3> idFillOrder() const override { return m_fillOrder; }
  int idstep() const override { return m_idstep; }
  std::shared_ptr<Detector> referentDetector() const override { return nullptr; }

private:
  size_t m_nx, m_ny, m_nz;
  double m_xstart, m_ystart, m_zstart;
  double m_xstep, m_ystep, m_zstep;
  detid_t m_idstart;
  std::array<char, 3> m_fillOrder;
  int m_idstep;
};

} // anonymous namespace

class IVirtualBankTest : public CxxTest::TestSuite {
public:
  // ---- helpers -------------------------------------------------------------
  /// Build a 4 × 5 × 3 bank with fill order X-fastest.
  static TestBank makeXYZ() {
    // 4 cols, 5 rows, 3 layers; step = 1 everywhere; IDs start at 1000.
    return TestBank(4, 5, 3,           // nx, ny, nz
                    -2.0, -2.5, 0.0,   // xstart, ystart, zstart
                    1.0, 1.0, 1.0,     // xstep, ystep, zstep
                    1000,              // idstart
                    {'x', 'y', 'z'}, 1 // fill order, idstep
    );
  }

  /// Build a 4 × 5 × 1 bank with fill order Y-fastest (like the old
  /// GridDetector idfillbyfirst_y = true convention).
  static TestBank makeYXZ() {
    return TestBank(4, 5, 1,           // nx, ny, nz
                    0.0, 0.0, 0.0,     // xstart, ystart, zstart
                    1.0, 1.0, 0.0,     // xstep, ystep, zstep
                    0,                 // idstart
                    {'y', 'x', 'z'}, 1 // fill order, idstep
    );
  }

  // ---- npixels -------------------------------------------------------------
  void testNpixels() {
    auto b = makeXYZ();
    TS_ASSERT_EQUALS(b.npixels(), 4u * 5u * 3u);
  }

  // ---- size accessors ------------------------------------------------------
  void testSize() {
    auto b = makeXYZ();
    TS_ASSERT_DELTA(b.xsize(), 4.0, 1e-10);
    TS_ASSERT_DELTA(b.ysize(), 5.0, 1e-10);
    TS_ASSERT_DELTA(b.zsize(), 3.0, 1e-10);
  }

  // ---- inBoundsXYZ ---------------------------------------------------------
  void testInBoundsXYZ() {
    auto b = makeXYZ(); // 4×5×3
    TS_ASSERT(b.inBoundsXYZ(0, 0, 0));
    TS_ASSERT(b.inBoundsXYZ(3, 4, 2));
    TS_ASSERT(!b.inBoundsXYZ(-1, 0, 0));
    TS_ASSERT(!b.inBoundsXYZ(0, -1, 0));
    TS_ASSERT(!b.inBoundsXYZ(0, 0, -1));
    TS_ASSERT(!b.inBoundsXYZ(4, 0, 0)); // == xpixels(), out of range
    TS_ASSERT(!b.inBoundsXYZ(0, 5, 0)); // == ypixels()
    TS_ASSERT(!b.inBoundsXYZ(0, 0, 3)); // == zpixels()
  }

  // ---- minDetectorID / maxDetectorID ---------------------------------------
  void testIDRange() {
    auto b = makeXYZ(); // 4×5×3, idstart=1000, fill {'x','y','z'}, step 1
    TS_ASSERT_EQUALS(b.minDetectorID(), 1000);
    // max is at (3, 4, 2): offset = 3*1 + 4*4 + 2*(4*5) = 3 + 16 + 40 = 59
    TS_ASSERT_EQUALS(b.maxDetectorID(), 1059);
  }

  // ---- getDetectorIDAtXYZ: X-fastest fill order ----------------------------
  void testGetIDAtXYZ_XFastest() {
    auto b = makeXYZ(); // idstart=1000, fill={'x','y','z'}, step=1, nx=4, ny=5
    // Referent
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 0, 0), 1000);
    // Move one step in X (fastest axis)
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(1, 0, 0), 1001);
    // Move one step in Y (middle axis): stride = nx = 4
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 1, 0), 1004);
    // Move one step in Z (slowest axis): stride = nx*ny = 20
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 0, 1), 1020);
    // Combined
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(2, 3, 1), 1000 + 2 + 3 * 4 + 1 * 20);
  }

  // ---- getDetectorIDAtXYZ: Y-fastest fill order ----------------------------
  void testGetIDAtXYZ_YFastest() {
    auto b = makeYXZ(); // idstart=0, fill={'y','x','z'}, step=1, nx=4, ny=5, nz=1
    // Referent
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 0, 0), 0);
    // Move one step in Y (fastest): stride = 1
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 1, 0), 1);
    // Move one step in X (middle): stride = ny = 5
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(1, 0, 0), 5);
    // Combined
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(2, 3, 0), 2 * 5 + 3);
  }

  // ---- roundtrip: getXYZForDetectorID / getDetectorIDAtXYZ ----------------
  void testRoundtripXFastest() {
    auto b = makeXYZ();
    // Check every pixel
    for (int z = 0; z < 3; ++z) {
      for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 4; ++x) {
          detid_t id = b.getDetectorIDAtXYZ(x, y, z);
          auto [rx, ry, rz] = b.getXYZForDetectorID(id);
          TS_ASSERT_EQUALS(rx, x);
          TS_ASSERT_EQUALS(ry, y);
          TS_ASSERT_EQUALS(rz, z);
        }
      }
    }
  }

  void testRoundtripYFastest() {
    auto b = makeYXZ();
    for (int x = 0; x < 4; ++x) {
      for (int y = 0; y < 5; ++y) {
        detid_t id = b.getDetectorIDAtXYZ(x, y, 0);
        auto [rx, ry, rz] = b.getXYZForDetectorID(id);
        TS_ASSERT_EQUALS(rx, x);
        TS_ASSERT_EQUALS(ry, y);
        TS_ASSERT_EQUALS(rz, 0);
      }
    }
  }

  // ---- getRelativePosAtXYZ ------------------------------------------------
  void testRelativePos() {
    auto b = makeXYZ(); // xstart=-2, ystart=-2.5, zstart=0; steps=1
    Kernel::V3D pos = b.getRelativePosAtXYZ(0, 0, 0);
    TS_ASSERT_DELTA(pos.X(), -2.0, 1e-10);
    TS_ASSERT_DELTA(pos.Y(), -2.5, 1e-10);
    TS_ASSERT_DELTA(pos.Z(), 0.0, 1e-10);

    pos = b.getRelativePosAtXYZ(3, 4, 2);
    TS_ASSERT_DELTA(pos.X(), -2.0 + 3.0, 1e-10);
    TS_ASSERT_DELTA(pos.Y(), -2.5 + 4.0, 1e-10);
    TS_ASSERT_DELTA(pos.Z(), 0.0 + 2.0, 1e-10);
  }

  // ---- idstep > 1 ---------------------------------------------------------
  void testIdstepGreaterThanOne() {
    // 3×3×1 grid, idstep=2, idstart=0, fill {'x','y','z'}
    TestBank b(3, 3, 1, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0, {'x', 'y', 'z'}, 2);
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 0, 0), 0);
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(1, 0, 0), 2);  // step=2
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(0, 1, 0), 6);  // stride Y = 3*2=6
    TS_ASSERT_EQUALS(b.getDetectorIDAtXYZ(2, 2, 0), 16); // 2*2 + 2*6 = 4+12
    TS_ASSERT_EQUALS(b.maxDetectorID(), 16);

    auto [rx, ry, rz] = b.getXYZForDetectorID(16);
    TS_ASSERT_EQUALS(rx, 2);
    TS_ASSERT_EQUALS(ry, 2);
    TS_ASSERT_EQUALS(rz, 0);
  }
};
