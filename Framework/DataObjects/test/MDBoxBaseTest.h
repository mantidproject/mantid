// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CoordTransform.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Timer.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using Mantid::Kernel::ConfigService;

/** Tester class that implements the minimum MDBoxBase to
 * allow testing
 */
TMDE_CLASS
class MDBoxBaseTester : public MDBoxBase<MDE, nd> {
public:
  MDBoxBaseTester() : MDBoxBase<MDE, nd>() {}
  ~MDBoxBaseTester() override = default;
  MDBoxBaseTester(uint64_t /*filePos*/) : MDBoxBase<MDE, nd>() {}
  MDBoxBaseTester(const MDBoxBaseTester &source) : MDBoxBase<MDE, nd>(source, source.getBoxController()) {}

  MDBoxBaseTester(const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector)
      : MDBoxBase<MDE, nd>(nullptr, 0, 0, extentsVector) {}
  //-----------------------------------------------------------------------------------------------
  Kernel::ISaveable *getISaveable() override { return nullptr; }
  Kernel::ISaveable *getISaveable() const override { return nullptr; }
  void setFileBacked(const uint64_t /*fileLocation*/, const size_t /*fileSize*/, const bool /*markSaved*/) override {};
  void clearFileBacked(bool /* loadData*/) override { /**does nothing*/ };
  void setFileBacked() override {};
  void saveAt(API::IBoxControllerIO *const, uint64_t /*position*/) const override { /*Not saveable */ };
  void loadAndAddFrom(API::IBoxControllerIO *const /*saver */, uint64_t /*position*/, size_t /* Size */,
                      std::vector<coord_t> & /*mem*/) override {};
  void loadAndAddFrom(API::IBoxControllerIO *const, uint64_t /*position*/, size_t /* Size */) override {};
  void reserveMemoryForLoad(uint64_t /* Size */) override {};
  // regardless of what is actually instantiated, base tester would call itself
  // gridbox
  bool isBox() const override { return false; }

  /// Clear all contained data
  void clear() override {}

  // Not to be pure virtual
  void getBoxes(std::vector<API::IMDNode *> &, const std::function<bool(API::IMDNode *)> &) override {}

  uint64_t getNPoints() const override {
    return 0;
    // return this->getFileSize();
  }
  size_t getDataInMemorySize() const override { return 0; }
  /// @return the amount of memory that the object takes up in the MRU.
  uint64_t getTotalDataSize() const override { return 0; }

  /// Get number of dimensions
  size_t getNumDims() const override { return nd; }

  /// Get the total # of unsplit MDBoxes contained.
  size_t getNumMDBoxes() const override { return 0; }

  size_t getNumChildren() const override { return 0; }

  MDBoxBase<MDE, nd> *getChild(size_t /*index*/) override { throw std::runtime_error("MDBox does not have children."); }

  /// Sets the children from a vector of children
  void setChildren(const std::vector<API::IMDNode *> & /*boxes*/, const size_t /*indexStart*/,
                   const size_t /*indexEnd*/) override {
    throw std::runtime_error("MDBox cannot have children.");
  }

  /// Return a copy of contained events
  std::vector<MDE> *getEventsCopy() override { return nullptr; }

  /// Add a single event
  size_t addEvent(const MDE & /*point*/) override { return 0; }
  /// Add a single event and trace it if the box it has been added may need
  /// splitting
  virtual void addAndTraceEvent(const MDE & /*point*/, size_t /*index */) {}

  /// Add a single event
  size_t addEventUnsafe(const MDE & /*point*/) override { return 0; }
  size_t buildAndAddEvents(const std::vector<signal_t> & /*sigErrSq*/, const std::vector<coord_t> & /*Coord*/,
                           const std::vector<uint16_t> & /*expInfoIndex*/,
                           const std::vector<uint16_t> & /*goniometerIndex*/,
                           const std::vector<uint32_t> & /*detectorId*/) override {
    return 0;
  }
  void buildAndAddEvent(const Mantid::signal_t, const Mantid::signal_t, const std::vector<coord_t> &, uint16_t,
                        uint16_t, uint32_t) override {};
  virtual void buildAndTraceEvent(const Mantid::signal_t, const Mantid::signal_t, const std::vector<coord_t> &,
                                  uint16_t, uint16_t, uint32_t, size_t) {};
  void buildAndAddEventUnsafe(const Mantid::signal_t, const Mantid::signal_t, const std::vector<coord_t> &, uint16_t,
                              uint16_t, uint32_t) override {};

  /** Perform centerpoint binning of events
   * @param bin :: MDBin object giving the limits of events to accept.
   */
  void centerpointBin(MDBin<MDE, nd> & /*bin*/, bool *) const override {}
  void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = nullptr) override {};
  void refreshCache(Kernel::ThreadScheduler * /*ts*/ = nullptr) override {};
  // virtual void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL){};
  void calculateCentroid(coord_t * /*centroid*/) const override {};
  void calculateCentroid(coord_t * /*centroid*/, const int /*expInfoIndex*/) const override {};
  coord_t *getCentroid() const override { return nullptr; };
  void integrateSphere(Mantid::API::CoordTransform & /*radiusTransform*/, const coord_t /*radiusSquared*/,
                       signal_t & /*signal*/, signal_t & /*errorSquared*/, const coord_t /*innerRadiusSquared*/,
                       const bool /*useOnePercentBackgroundCorrection*/) const override {};
  void centroidSphere(Mantid::API::CoordTransform & /*radiusTransform*/, const coord_t /*radiusSquared*/, coord_t *,
                      signal_t &) const override {};
  void integrateCylinder(Mantid::API::CoordTransform & /*radiusTransform*/, const coord_t /*radius*/,
                         const coord_t /*length*/, signal_t & /*signal*/, signal_t & /*errorSquared*/,
                         std::vector<signal_t> & /*signal_fit*/) const override {};
  void getBoxes(std::vector<API::IMDNode *> & /*boxes*/, size_t /*maxDepth*/, bool) override {};
  void getBoxes(std::vector<API::IMDNode *> & /*boxes*/, size_t /*maxDepth*/, bool,
                Mantid::Geometry::MDImplicitFunction *) override {};

  void generalBin(MDBin<MDE, nd> & /*bin*/, Mantid::Geometry::MDImplicitFunction & /*function*/) const override {}
  void clearDataFromMemory() override {};

  bool getIsMasked() const override { throw std::runtime_error("MDBoxBaseTester does not implement getIsMasked"); }

  void mask() override { throw std::runtime_error("MDBoxBaseTester does not implement mask"); }

  void unmask() override { throw std::runtime_error("MDBoxBaseTester does not implement unmask"); }
};

class MDBoxBaseTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> box;
    TS_ASSERT_EQUALS(box.getSignal(), 0.0);
    TS_ASSERT_EQUALS(box.getErrorSquared(), 0.0);
  }

  void test_extents_constructor() {
    using ibox3 = MDBoxBaseTester<MDLeanEvent<3>, 3>;
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extentsVector;
    TS_ASSERT_THROWS_ANYTHING(ibox3 box(extentsVector));
    extentsVector.resize(3);
    for (size_t d = 0; d < 3; d++) {
      extentsVector[d].setExtents(static_cast<double>(d) + 0.1, static_cast<double>(d + 1));
    }
    MDBoxBaseTester<MDLeanEvent<3>, 3> box(extentsVector);
    TS_ASSERT_DELTA(box.getExtents(0).getMin(), 0.1, 1e-4);
    TS_ASSERT_DELTA(box.getExtents(0).getMax(), 1.0, 1e-4);
    TS_ASSERT_DELTA(box.getExtents(1).getMin(), 1.1, 1e-4);
    TS_ASSERT_DELTA(box.getExtents(1).getMax(), 2.0, 1e-4);
    TS_ASSERT_DELTA(box.getExtents(2).getMin(), 2.1, 1e-4);
    TS_ASSERT_DELTA(box.getExtents(2).getMax(), 3.0, 1e-4);
  }

  void test_transformDimensions() {
    using ibox3 = MDBoxBaseTester<MDLeanEvent<2>, 2>;
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extentsVector;
    TS_ASSERT_THROWS_ANYTHING(ibox3 box(extentsVector));
    extentsVector.resize(2);
    for (size_t d = 0; d < 2; d++) {
      extentsVector[d].setExtents(1, 2);
    }
    MDBoxBaseTester<MDLeanEvent<2>, 2> box(extentsVector);
    // Now transform
    std::vector<double> scaling(2, 3.0);
    std::vector<double> offset(2, 1.0);
    box.transformDimensions(scaling, offset);
    for (size_t d = 0; d < 2; d++) {
      TS_ASSERT_DELTA(box.getExtents(d).getMin(), 4.0, 1e-4);
      TS_ASSERT_DELTA(box.getExtents(d).getMax(), 7.0, 1e-4);
    }
    TS_ASSERT_DELTA(box.getVolume(), 9.0, 1e-4);
  }

  void test_get_and_set_signal() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> box;
    TS_ASSERT_EQUALS(box.getSignal(), 0.0);
    TS_ASSERT_EQUALS(box.getErrorSquared(), 0.0);
    box.setSignal(123.0);
    box.setErrorSquared(456.0);
    TS_ASSERT_EQUALS(box.getSignal(), 123.0);
    TS_ASSERT_EQUALS(box.getErrorSquared(), 456.0);
    TS_ASSERT_DELTA(box.getError(), sqrt(456.0), 1e-4);
  }

  void test_getTotalWeight() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> box;
    TS_ASSERT_EQUALS(box.getTotalWeight(), 0.0);
    box.setTotalWeight(123.0);
    TS_ASSERT_EQUALS(box.getTotalWeight(), 123.0);
  }

  void test_get_and_set_depth() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    b.setDepth(123);
    TS_ASSERT_EQUALS(b.getDepth(), 123);
  }

  void test_getBoxAtCoord() {
    coord_t dummy[3] = {1, 2, 3};
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    TSM_ASSERT_EQUALS("MDBoxBase->getBoxAtCoord() always returns this.", b.getBoxAtCoord(dummy), &b);
  }

  void test_getParent_and_setParent() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    TSM_ASSERT("Default parent is NULL", !b.getParent());
    MDBoxBaseTester<MDLeanEvent<3>, 3> *daddy = new MDBoxBaseTester<MDLeanEvent<3>, 3>;
    b.setParent(daddy);
    TS_ASSERT_EQUALS(b.getParent(), daddy);
    // Copy ctor
    MDBoxBaseTester<MDLeanEvent<3>, 3> c(b);
    TS_ASSERT_EQUALS(c.getParent(), daddy);

    delete daddy;
  }

  /** Setting and getting the extents;
   * also, getting the center */
  void test_setExtents() {
    MDBoxBaseTester<MDLeanEvent<2>, 2> b;
    b.setExtents(0, -8.0, 10.0);
    TS_ASSERT_DELTA(b.getExtents(0).getMin(), -8.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(0).getMax(), +10.0, 1e-6);

    b.setExtents(1, -4.0, 12.0);
    TS_ASSERT_DELTA(b.getExtents(1).getMin(), -4.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(1).getMax(), +12.0, 1e-6);

    TS_ASSERT_THROWS(b.setExtents(2, 0, 1.0), const std::invalid_argument &);

    coord_t center[2];
    b.getCenter(center);
    TS_ASSERT_DELTA(center[0], +1.0, 1e-6);
    TS_ASSERT_DELTA(center[1], +4.0, 1e-6);
  }

  void test_copy_constructor() {
    MDBoxBaseTester<MDLeanEvent<2>, 2> b;
    b.setDepth(6);
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.setSignal(123.0);
    b.setErrorSquared(456.0);
    b.setID(8765);
    b.calcVolume();

    // Perform the copy
    MDBoxBaseTester<MDLeanEvent<2>, 2> box(b);
    TS_ASSERT_DELTA(box.getExtents(0).getMin(), -10.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(0).getMax(), +10.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(1).getMin(), -4.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(1).getMax(), +6.0, 1e-6);
    TS_ASSERT_DELTA(box.getSignal(), b.getSignal(), 1e-6);
    TS_ASSERT_DELTA(box.getErrorSquared(), b.getErrorSquared(), 1e-6);
    TS_ASSERT_DELTA(box.getInverseVolume(), b.getInverseVolume(), 1e-6);
    TS_ASSERT_EQUALS(box.getID(), b.getID());
    TS_ASSERT_EQUALS(box.getDepth(), b.getDepth());
  }

  /** Calculating volume and normalizing signal by it. */
  void test_calcVolume() {
    MDBoxBaseTester<MDLeanEvent<2>, 2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.calcVolume();
    TS_ASSERT_DELTA(b.getVolume(), 200.0, 1e-5);
    TS_ASSERT_DELTA(b.getInverseVolume(), 1.0 / 200.0, 1e-5);

    b.setSignal(100.0);
    b.setErrorSquared(300.0);

    TS_ASSERT_DELTA(b.getSignal(), 100.0, 1e-5);
    TS_ASSERT_DELTA(b.getSignalNormalized(), 0.5, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 300.0, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquaredNormalized(), 1.5, 1e-5);
  }

  /** Get vertexes using the extents */
  void test_getVertexes() {
    MDBoxBaseTester<MDLeanEvent<2>, 2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    std::vector<Mantid::Kernel::VMD> v = b.getVertexes();
    TS_ASSERT_EQUALS(v[0][0], -10.0);
    TS_ASSERT_EQUALS(v[0][1], -4.0);
    TS_ASSERT_EQUALS(v[1][0], 10.0);
    TS_ASSERT_EQUALS(v[1][1], -4.0);
    TS_ASSERT_EQUALS(v[2][0], -10.0);
    TS_ASSERT_EQUALS(v[2][1], 6.0);
    TS_ASSERT_EQUALS(v[3][0], 10.0);
    TS_ASSERT_EQUALS(v[3][1], 6.0);
  }

  /** Get vertexes as a bare array */
  void test_getVertexesArray() {
    MDBoxBaseTester<MDLeanEvent<2>, 2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    size_t numVertexes = 0;
    auto v = b.getVertexesArray(numVertexes);
    TS_ASSERT_EQUALS(numVertexes, 4);
    TS_ASSERT_EQUALS(v[0], -10.0);
    TS_ASSERT_EQUALS(v[0 + 1], -4.0);
    TS_ASSERT_EQUALS(v[2], 10.0);
    TS_ASSERT_EQUALS(v[2 + 1], -4.0);
    TS_ASSERT_EQUALS(v[4], -10.0);
    TS_ASSERT_EQUALS(v[4 + 1], 6.0);
    TS_ASSERT_EQUALS(v[6], 10.0);
    TS_ASSERT_EQUALS(v[6 + 1], 6.0);
  }

  /** Get vertexes as a bare array,
   * projecting down into fewer dimensions */
  void test_getVertexesArray_reducedDimension() {
    MDBoxBaseTester<MDLeanEvent<2>, 2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    size_t numVertexes = 0;

    bool maskDim[2] = {true, false};
    auto v = b.getVertexesArray(numVertexes, 1, maskDim);
    TS_ASSERT_EQUALS(numVertexes, 2);
    TS_ASSERT_EQUALS(v[0], -10.0);
    TS_ASSERT_EQUALS(v[1], 10.0);

    bool maskDim2[2] = {false, true};
    v = b.getVertexesArray(numVertexes, 1, maskDim2);
    TS_ASSERT_EQUALS(numVertexes, 2);
    TS_ASSERT_EQUALS(v[0], -4.0);
    TS_ASSERT_EQUALS(v[1], 6.0);
  }

  /** Get vertexes as a bare array,
   * projecting down into fewer dimensions */
  void test_getVertexesArray_reducedDimension_3D() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.setExtents(2, -2.0, 8.0);
    size_t numVertexes = 0;

    // 3D projected down to 2D in X/Y
    bool maskDim[3] = {true, true, false};
    auto v = b.getVertexesArray(numVertexes, 2, maskDim);
    TS_ASSERT_EQUALS(numVertexes, 4);
    TS_ASSERT_EQUALS(v[0], -10.0);
    TS_ASSERT_EQUALS(v[0 + 1], -4.0);
    TS_ASSERT_EQUALS(v[2], 10.0);
    TS_ASSERT_EQUALS(v[2 + 1], -4.0);
    TS_ASSERT_EQUALS(v[4], -10.0);
    TS_ASSERT_EQUALS(v[4 + 1], 6.0);
    TS_ASSERT_EQUALS(v[6], 10.0);
    TS_ASSERT_EQUALS(v[6 + 1], 6.0);

    // Can't give 0 dimensions.
    TS_ASSERT_THROWS_ANYTHING(v = b.getVertexesArray(numVertexes, 0, maskDim));

    // 3D projected down to 1D in Y
    bool maskDim2[3] = {false, true, false};
    v = b.getVertexesArray(numVertexes, 1, maskDim2);
    TS_ASSERT_EQUALS(numVertexes, 2);
    TS_ASSERT_EQUALS(v[0], -4.0);
    TS_ASSERT_EQUALS(v[1], 6.0);

    // 3D projected down to 2D in Y/Z
    bool maskDim3[3] = {false, true, true};
    v = b.getVertexesArray(numVertexes, 2, maskDim3);
    TS_ASSERT_EQUALS(numVertexes, 4);
    TS_ASSERT_EQUALS(v[0], -4.0);
    TS_ASSERT_EQUALS(v[0 + 1], -2.0);
    TS_ASSERT_EQUALS(v[2], 6.0);
    TS_ASSERT_EQUALS(v[2 + 1], -2.0);
    TS_ASSERT_EQUALS(v[4], -4.0);
    TS_ASSERT_EQUALS(v[4 + 1], 8.0);
    TS_ASSERT_EQUALS(v[6], 6.0);
    TS_ASSERT_EQUALS(v[6 + 1], 8.0);
  }

  void xtest_sortBoxesByFilePos() {
    //    std::vector<API::IMDNode *> boxes;
    //    // 10 to 1 in reverse order
    //    for (uint64_t i=0; i<10; i++)
    //    {
    //      boxes.emplace_back(new MDBoxBaseTester<MDLeanEvent<1>,1>(10-i));
    //    }
    // TODO:
    // Kernel::ISaveable::sortObjByFilePos(boxes);
    //// After sorting, they are in the right order 1,2,3, etc.
    // for (uint64_t i=0; i<10; i++)
    //{
    //    TS_ASSERT_EQUALS( boxes[i]->getFilePosition(), i+1);
    //    delete boxes[i];
    //}
  }
};

//======================================================================
//======================================================================
//======================================================================
class MDBoxBaseTestPerformance : public CxxTest::TestSuite {
public:
  /** Vector of VMD version of getVertexes (slower than the bare array version)
   * (this is only 100 thousand, not a million times like the others).
   */
  void test_getVertexes_3D() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    for (size_t i = 0; i < 100000; i++) {
      std::vector<Mantid::Kernel::VMD> v = b.getVertexes();
    }
  }

  void test_getVertexesArray_3D() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    for (size_t i = 0; i < 1000000; i++) {
      size_t numVertexes;
      auto v = b.getVertexesArray(numVertexes);
    }
  }

  void test_getVertexesArray_3D_projected_to_2D() {
    MDBoxBaseTester<MDLeanEvent<3>, 3> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    bool maskDim[3] = {true, true, false};
    for (size_t i = 0; i < 1000000; i++) {
      size_t numVertexes;
      auto v = b.getVertexesArray(numVertexes, 2, maskDim);
    }
  }

  void test_getVertexesArray_4D() {
    MDBoxBaseTester<MDLeanEvent<4>, 4> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    b.setExtents(3, -6.0, 6.0);
    for (size_t i = 0; i < 1000000; i++) {
      size_t numVertexes;
      auto v = b.getVertexesArray(numVertexes);
    }
  }
  void test_getVertexesArray_4D_projected_to_3D() {
    MDBoxBaseTester<MDLeanEvent<4>, 4> b;
    bool maskDim[4] = {true, true, true, false};
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    b.setExtents(3, -6.0, 6.0);
    for (size_t i = 0; i < 1000000; i++) {
      size_t numVertexes;
      auto v = b.getVertexesArray(numVertexes, 3, maskDim);
    }
  }
};
