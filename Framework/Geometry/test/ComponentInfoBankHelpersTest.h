#ifndef MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_
#define MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::Geometry::ComponentInfoBankHelpers;

class ComponentInfoBankHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoBankHelpersTest *createSuite() {
    return new ComponentInfoBankHelpersTest();
  }
  static void destroySuite(ComponentInfoBankHelpersTest *suite) {
    delete suite;
  }

  void test_DetectorFixedInBankTrueForRectangularBank() {
    auto rectInstr =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 2);
    auto wrappers = InstrumentVisitor::makeWrappers(*rectInstr);
    const auto &componentInfo = *wrappers.first;

    TS_ASSERT(isDetectorFixedInBank(componentInfo, 0));
  }

  void test_DetectorFixedInBankFalseForNonStructuredBank() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &componentInfo = *wrappers.first;

    TS_ASSERT(!isDetectorFixedInBank(componentInfo, 0));
  }

  void test_DetectorFixedInBankFalseForMonitor() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto det = new Detector("MyTestMonitor", 10, instr.get());
    auto shape = ComponentCreationHelper::createCuboid(0.001, 0.001, 0.001);
    det->setShape(shape);
    det->setPos(V3D(0, 0, 0));
    det->setRot(Quat());

    instr->add(det);
    instr->markAsMonitor(det);

    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &componentInfo = *wrappers.first;

    auto index = componentInfo.indexOfAny("MyTestMonitor");

    TS_ASSERT(!isDetectorFixedInBank(componentInfo, index));
  }

  void test_DetectorFixedInBankFalseForNonDetectorComponent() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto wrappers = InstrumentVisitor::makeWrappers(*instr);
    const auto &componentInfo = *wrappers.first;

    TS_ASSERT(!isDetectorFixedInBank(componentInfo, componentInfo.root()));
  }

  void test_isGridDetectorPixelReturnsTrueForGridOrRectangleBankDetector() {
    auto rectInstr =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 12);
    auto gridInstr = ComponentCreationHelper::createMinimalInstrument(
        V3D(), V3D(0, 0, 1), V3D(0, 0, 1));

    auto grid = createGridBank();
    gridInstr->add(grid);
    markGridDetectors(gridInstr, grid);

    auto rectWrappers = InstrumentVisitor::makeWrappers(*rectInstr);
    const auto &rectComponentInfo = *rectWrappers.first;
    auto gridWrappers = InstrumentVisitor::makeWrappers(*gridInstr);
    const auto &gridComponentInfo = *gridWrappers.first;

    TS_ASSERT(isGridDetectorPixel(rectComponentInfo, 10));
    TS_ASSERT(isGridDetectorPixel(gridComponentInfo, 10));

    // returns false for anything else
    TS_ASSERT(
        !isGridDetectorPixel(rectComponentInfo, rectComponentInfo.root()));
  }

private:
  void markGridDetectors(Instrument_sptr instr, GridDetector *grid) {
    for (int z = 0; z < grid->nelements(); ++z) {
      auto layer =
          boost::dynamic_pointer_cast<Geometry::ICompAssembly>((*grid)[z]);
      for (int x = 0; x < layer->nelements(); ++x) {
        auto column =
            boost::dynamic_pointer_cast<Geometry::ICompAssembly>((*layer)[x]);
        for (int y = 0; y < column->nelements(); ++y) {
          auto det =
              boost::dynamic_pointer_cast<Geometry::Detector>((*column)[y]);
          if (det)
            instr->markAsDetectorIncomplete(det.get());
        }
      }
    }
  }

  GridDetector *createGridBank() {
    auto grid = new GridDetector("GridDetector");
    auto shape = ComponentCreationHelper::createCuboid(0.005, 0.005, 0.005);
    grid->initialize(shape, 10, 0, 0.005, 10, 0, 0.005, 10, 0, 0.005, 100,
                     "xyz", 10);

    return grid;
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERSTEST_H_ */