#ifndef MANTID_GEOMETRY_HKLFILTERTEST_H_
#define MANTID_GEOMETRY_HKLFILTERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidKernel/Timer.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class HKLFilterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLFilterTest *createSuite() { return new HKLFilterTest(); }
  static void destroySuite(HKLFilterTest *suite) { delete suite; }

  void test_Something() {
    UnitCell cellAl2O3(4.759355, 4.759355, 12.99231, 90.0, 90.0, 120.0);
    CompositeBraggScatterer_sptr scatterers = CompositeBraggScatterer::create();
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=Al;Position=[0,0,0.35217];U=0.005"));
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=O;Position=[0.69365,0,0.25];U=0.005"));
    SpaceGroup_const_sptr sgAl2O3 =
        SpaceGroupFactory::Instance().createSpaceGroup("R -3 c");

    CrystalStructure mg(cellAl2O3, sgAl2O3, scatterers);

    HKLFilterDRange dFilter(mg.cell(), 0.7, 200.0);
    HKLFilterCentering centering(mg.centering());
    HKLFilterSpaceGroup sgFilter(mg.spaceGroup());
  }

private:
  void inspectFilter(const HKLFilter_const_sptr &filter) {
    std::cout << filter->getName() << std::endl;

    try {
      boost::shared_ptr<const HKLFilterBinaryLogicOperation> op =
          boost::dynamic_pointer_cast<const HKLFilterBinaryLogicOperation>(
              filter);
      std::cout << "LHS: ";
      inspectFilter(op->getLHS());

      std::cout << "RHS: ";
      inspectFilter(op->getRHS());
    } catch (std::bad_cast) {
      // nothing.
    }
  }
};
#endif /* MANTID_GEOMETRY_HKLFILTERTEST_H_ */
