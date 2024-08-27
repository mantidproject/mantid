// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidAPI/IBoxControllerIO.h"
#include "MantidFrameworkTestHelpers/BoxControllerDummyIO.h"
#include "MantidKernel/DiskBuffer.h"
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class BoxControllerTest : public CxxTest::TestSuite {
public:
  void test_Constructor() {
    BoxController sc(2);
    TS_ASSERT_EQUALS(sc.getNDims(), 2);
  }

  void test_willSplit() {
    BoxController sc(2);
    sc.setMaxDepth(4);
    sc.setSplitThreshold(10);
    TS_ASSERT(sc.willSplit(100, 3));
    TS_ASSERT(!sc.willSplit(100, 4));
    TS_ASSERT(!sc.willSplit(2, 3));
    TS_ASSERT(!sc.willSplit(100, 5));
  }

  void test_getSplitInto() {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS(sc.getNumSplit(), 1000);
    TS_ASSERT_EQUALS(sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS(sc.getSplitInto(1), 10);
    TS_ASSERT_EQUALS(sc.getSplitInto(2), 10);
    sc.setSplitInto(1, 5);
    TS_ASSERT_EQUALS(sc.getNumSplit(), 500);
    TS_ASSERT_EQUALS(sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS(sc.getSplitInto(1), 5);
    TS_ASSERT_EQUALS(sc.getSplitInto(2), 10);
  }

  void test_maxDepth() {
    BoxController sc(3);
    sc.setSplitInto(10);
    sc.setMaxDepth(6);
    TS_ASSERT_EQUALS(sc.getMaxDepth(), 6);
  }

  void test_IDs() {
    BoxController sc(3);
    // No IDs given out yet
    TS_ASSERT_EQUALS(sc.getMaxId(), 0);
    // Start handing some out
    TS_ASSERT_EQUALS(sc.getNextId(), 0);
    TS_ASSERT_EQUALS(sc.getNextId(), 1);
    TS_ASSERT_EQUALS(sc.getNextId(), 2);
    // You've got 3 given.
    TS_ASSERT_EQUALS(sc.getMaxId(), 3);

    sc.setSplitInto(10);
    sc.setMaxDepth(6);
    TS_ASSERT_EQUALS(sc.getMaxDepth(), 6);
  }

  void test_maxNumBoxes() {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS(sc.getNumSplit(), 1000);
    sc.setMaxDepth(6);
    {
      const std::vector<double> &max = sc.getMaxNumMDBoxes();
      TS_ASSERT_DELTA(max[0], 1.0, 1e-2);
      TS_ASSERT_DELTA(max[1], 1e3, 1e-2);
      TS_ASSERT_DELTA(max[2], 1e6, 1e-2);
      TS_ASSERT_DELTA(max[3], 1e9, 1e-2);
    }

    {
      // If you split into a different number, the values get reset too.
      sc.setSplitInto(5);
      TS_ASSERT_EQUALS(sc.getNumSplit(), 125);
      const std::vector<double> &max = sc.getMaxNumMDBoxes();
      TS_ASSERT_DELTA(max[0], 1.0, 1e-2);
      TS_ASSERT_DELTA(max[1], 125.0, 1e-2);
      TS_ASSERT_DELTA(max[2], 125 * 125.0, 1e-2);
    }
  }

  void test_setSplitTopIntoWorksCorrectly() {
    BoxController sc(3);
    sc.setSplitTopInto(0, 10);
    sc.setSplitTopInto(1, 20);
    sc.setSplitTopInto(2, 30);

    std::optional<std::vector<size_t>> splitTopInto = sc.getSplitTopInto();

    TSM_ASSERT_EQUALS("Should have three dimensions", splitTopInto.value().size(), 3);
    TSM_ASSERT_EQUALS("Should have a value of 10 in the first dimension", splitTopInto.value()[0], 10);
    TSM_ASSERT_EQUALS("Should have a value of 20 in the second dimension", splitTopInto.value()[1], 20);
    TSM_ASSERT_EQUALS("Should have a value of 30 in the third dimension", splitTopInto.value()[2], 30);
  }

  void test_setSplitTopIntoThrowsForWrongDimension() {
    BoxController sc(1);
    TSM_ASSERT_THROWS("Should throw for setting a wrong dimension", sc.setSplitTopInto(1, 10),
                      const std::invalid_argument &);
  }

  void doTest_numBoxes(BoxController &bc, size_t expectedNumEntries) {
    const std::vector<size_t> &num = bc.getNumMDBoxes();
    TS_ASSERT_EQUALS(num.size(), expectedNumEntries);
    TS_ASSERT_EQUALS(num[0], 1);
    TS_ASSERT_EQUALS(num[1], 0);

    // Average depth is 0 = all at level 0.
    TS_ASSERT_DELTA(bc.getAverageDepth(), 0.0, 1e-5);

    bc.trackNumBoxes(0);
    TS_ASSERT_EQUALS(num[0], 0);
    TS_ASSERT_EQUALS(num[1], 100);

    // All at depth 1.0
    TS_ASSERT_DELTA(bc.getAverageDepth(), 1.0, 1e-5);

    bc.trackNumBoxes(1);
    bc.trackNumBoxes(1);
    TS_ASSERT_EQUALS(num[0], 0);
    TS_ASSERT_EQUALS(num[1], 98);
    TS_ASSERT_EQUALS(num[2], 200);

    // Mostly at depth 1.0
    TS_ASSERT_DELTA(bc.getAverageDepth(), 1.02, 1e-5);
  }

  /* Try setting these values in different orders */
  void test_trackNumBoxes1() {
    BoxController bc(2);
    bc.setSplitInto(10);
    bc.setMaxDepth(4);
    doTest_numBoxes(bc, 5);
  }

  /* This used to give wrong values */
  void test_trackNumBoxes2() {
    BoxController bc(2);
    bc.setMaxDepth(4);
    bc.setSplitInto(10);
    bc.setMaxDepth(10);
    doTest_numBoxes(bc, 11);
  }

  /// Make sure that the correct number of boxes are recorded when we use
  /// splitting
  void test_trackNumBoxesWithTopLevelSplitting() {
    BoxController bc(2);
    bc.setSplitInto(10);

    bc.setSplitTopInto(0, 4);
    bc.setSplitTopInto(1, 12);

    // This includes a forced top level split and a subsequent split of two
    // boxes
    TSM_ASSERT_DELTA("The average depth should be 0", bc.getAverageDepth(), 0.0, 1e-5);
    bc.trackNumBoxes(0);
    TSM_ASSERT_DELTA("The average depth should be about 1", bc.getAverageDepth(), 1.0, 1e-5);

    bc.trackNumBoxes(1);
    bc.trackNumBoxes(1);

    const std::vector<size_t> &num = bc.getNumMDBoxes();
    const std::vector<size_t> &numGridBoxes = bc.getNumMDGridBoxes();
    TSM_ASSERT_EQUALS("Should be 1 MDGridBox structure at the 0th level", numGridBoxes[0], 1);
    TSM_ASSERT_EQUALS("Should be 48 - 2 MDBox structures at the 1st level", num[1], 46);
    TSM_ASSERT_EQUALS("Should be 2 MDGridBox structure at the 1st level", numGridBoxes[1], 2);
    TSM_ASSERT_EQUALS("Should be 2 * 100 MDBox structures at the 2nd level.", num[2], 200);
  }

  void test_trackNumBoxesWithTopLevelSplittingAndSettingMaxDepth() {
    BoxController bc(2);

    bc.setMaxDepth(4);
    bc.setSplitInto(10);

    bc.setSplitTopInto(0, 4);
    bc.setSplitTopInto(1, 12);
    bc.setMaxDepth(10);

    // This includes a forced top level split and a subsequent split of two
    // boxes
    TSM_ASSERT_DELTA("The average depth should be 0", bc.getAverageDepth(), 0.0, 1e-5);
    bc.trackNumBoxes(0);
    TSM_ASSERT_DELTA("The average depth should be about 1", bc.getAverageDepth(), 1.0, 1e-5);

    bc.trackNumBoxes(1);
    bc.trackNumBoxes(1);

    const std::vector<size_t> &num = bc.getNumMDBoxes();
    const std::vector<size_t> &numGridBoxes = bc.getNumMDGridBoxes();
    TSM_ASSERT_EQUALS("Should be 1 MDGridBox structure at the 0th level", numGridBoxes[0], 1);
    TSM_ASSERT_EQUALS("Should be 48 - 2 MDBox structures at the 1st level", num[1], 46);
    TSM_ASSERT_EQUALS("Should be 2 MDGridBox structure at the 1st level", numGridBoxes[1], 2);
    TSM_ASSERT_EQUALS("Should be 2 * 100 MDBox structures at the 2nd level.", num[2], 200);
  }

  /// Compare two box controllers and assert each part of them.
  void compareBoxControllers(BoxController &a, BoxController &b) {
    TS_ASSERT_EQUALS(a.getNDims(), b.getNDims());
    TS_ASSERT_EQUALS(a.getMaxDepth(), b.getMaxDepth());
    TS_ASSERT_EQUALS(a.getMaxId(), b.getMaxId());
    TS_ASSERT_EQUALS(a.getSplitThreshold(), b.getSplitThreshold());
    TS_ASSERT_EQUALS(a.getNumMDBoxes(), b.getNumMDBoxes());
    TS_ASSERT_EQUALS(a.getNumSplit(), b.getNumSplit());
    TS_ASSERT_EQUALS(a.getMaxNumMDBoxes(), b.getMaxNumMDBoxes());
    for (size_t d = 0; d < a.getNDims(); d++) {
      TS_ASSERT_EQUALS(a.getSplitInto(d), b.getSplitInto(d));
    }
    if (a.isFileBacked() && b.isFileBacked()) {
      TS_ASSERT_DIFFERS(a.getFileIO(), b.getFileIO());
    }

    // Check for top level splitting
    if (a.getSplitTopInto() && b.getSplitTopInto()) {
      for (size_t d = 0; d < a.getNDims(); d++) {
        TS_ASSERT_EQUALS(a.getSplitTopInto().value()[d], b.getSplitTopInto().value()[d]);
      }
    } else {
      TS_ASSERT_EQUALS(a.getSplitTopInto(), b.getSplitTopInto());
    }
  }

  /// Generate XML and read it back
  void test_xml() {
    BoxController a(2);
    a.setMaxDepth(4);
    a.setSplitInto(10);
    a.setMaxDepth(10);
    a.setMaxId(123456);

    std::string xml = a.toXMLString();
    TS_ASSERT(!xml.empty());

    // Read it back
    BoxController b(1);
    b.fromXMLString(xml);
    // Check that it is the same
    compareBoxControllers(a, b);
  }

  void test_xmlWithSplitTopIntoBeingSet() {
    BoxController a(2);
    a.setMaxDepth(4);
    a.setSplitInto(10);
    a.setMaxDepth(10);
    a.setMaxId(123456);
    TSM_ASSERT_THROWS_NOTHING("Should add the first dimension", a.setSplitTopInto(0, 10));
    TSM_ASSERT_THROWS_NOTHING("Should add the second dimension", a.setSplitTopInto(1, 20));

    std::string xml = a.toXMLString();
    TS_ASSERT(!xml.empty());

    // Read it back
    BoxController b(2);
    b.fromXMLString(xml);
    // Check that it is the same
    compareBoxControllers(a, b);
  }

  void test_Clone() {
    BoxController a(2);
    a.setMaxDepth(4);
    a.setSplitInto(10);
    a.setMaxDepth(10);
    a.setMaxId(123456);
    auto b = BoxController_sptr(a.clone());
    // Check that settings are the same but BC are different
    compareBoxControllers(a, *b);
  }

  void test_CloneWithSplitTopIntoBeingSet() {
    BoxController a(2);
    a.setMaxDepth(4);
    a.setSplitInto(10);
    a.setMaxDepth(10);
    a.setMaxId(123456);
    TSM_ASSERT_THROWS_NOTHING("Should add the first dimension", a.setSplitTopInto(0, 10));
    TSM_ASSERT_THROWS_NOTHING("Should add the second dimension", a.setSplitTopInto(1, 20));

    auto b = BoxController_sptr(a.clone());
    // Check that settings are the same but BC are different
    compareBoxControllers(a, *b);
  }

  void test_CloneFileBased() {
    auto a = std::make_shared<BoxController>(2);
    a->setMaxDepth(4);
    a->setSplitInto(10);
    a->setMaxDepth(10);
    a->setMaxId(123456);
    std::shared_ptr<IBoxControllerIO> pS(new MantidTestHelpers::BoxControllerDummyIO(a.get()));
    TS_ASSERT_THROWS_NOTHING(a->setFileBacked(pS, "fakeFile"));
    TS_ASSERT(a->isFileBacked());

    auto b = BoxController_sptr(a->clone());
    // Check that settings are the same but BC are different
    compareBoxControllers(*a, *b);

    TS_ASSERT(!b->isFileBacked());
    std::shared_ptr<IBoxControllerIO> pS2(new MantidTestHelpers::BoxControllerDummyIO(b.get()));
    TS_ASSERT_THROWS_NOTHING(b->setFileBacked(pS2, "fakeFile2"));

    // Check that settings are the same but BC are different
    compareBoxControllers(*a, *b);
    TS_ASSERT(b->isFileBacked());
  }

  void test_MRU_access() {
    auto a = std::make_shared<BoxController>(2);
    std::shared_ptr<IBoxControllerIO> pS(new MantidTestHelpers::BoxControllerDummyIO(a.get()));
    a->setFileBacked(pS, "existingFakeFile");
    DiskBuffer *dbuf = a->getFileIO();

    // Set the cache parameters
    TS_ASSERT_THROWS_NOTHING(dbuf->setWriteBufferSize(123));
    TS_ASSERT_EQUALS(dbuf->getWriteBufferSize(), 123);
  }

  void test_construction_defaults() {
    // Check the constructor defaults.
    BoxController box_controller(2);

    std::optional<std::vector<size_t>> splitTopInto = box_controller.getSplitTopInto();
    TS_ASSERT(!splitTopInto)
    TS_ASSERT_EQUALS(2, box_controller.getNDims());
    TS_ASSERT_EQUALS(1, box_controller.getNumSplit());
    TS_ASSERT_EQUALS(0, box_controller.getMaxId());
  }

  void test_openCloseFileBacked() {
    auto a = std::make_shared<BoxController>(2);
    TS_ASSERT(!a->isFileBacked());

    std::shared_ptr<IBoxControllerIO> pS(new MantidTestHelpers::BoxControllerDummyIO(a.get()));
    TS_ASSERT_THROWS_NOTHING(a->setFileBacked(pS, "fakeFile"));

    TSM_ASSERT("Box controller should have open faked file", pS->isOpened());
    std::string fileName = pS->getFileName();
    TSM_ASSERT_EQUALS("Box controller file should be named as requested ", 0,
                      fileName.compare(fileName.size() - 8, 8, std::string("fakeFile")));
    TS_ASSERT(a->isFileBacked());

    TS_ASSERT_THROWS_NOTHING(a->clearFileBacked());
    TS_ASSERT(!a->isFileBacked());
    TSM_ASSERT("Box controller should now close the faked file", !pS->isOpened());
  }
};
