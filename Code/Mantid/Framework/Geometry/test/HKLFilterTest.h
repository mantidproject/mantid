#ifndef MANTID_GEOMETRY_HKLFILTERTEST_H_
#define MANTID_GEOMETRY_HKLFILTERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/HKLFilter.h"
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
        SpaceGroupFactory::Instance().createSpaceGroup("C m m m");

    CrystalStructure mg(cellAl2O3, sgAl2O3, scatterers);

    HKLFilterDRange f1(0.7, 200.0);
    f1.setCrystalStructure(mg);

    HKLFilterCentering f15;
    f15.setCrystalStructure(mg);

    HKLFilterStructureFactor f2;
    f2.setCrystalStructure(mg);

    HKLFilterAnd f3 = f1 & f15 & f2;

    inspectFilter(f3);

    HKLGenerator g;

    Timer t;

    t.reset();
    for (size_t i = 0; i < 100; ++i) {
      std::vector<V3D> hkls =
          g.generateHKLs(mg, std::min(0.7, static_cast<double>(i + 2)), f3);
      TS_ASSERT_DIFFERS(hkls.size(), 0);
    }
    float time = t.elapsed();
    std::cout << time / 100.0 << std::endl;

    t.reset();
    for (size_t i = 0; i < 100; ++i) {
      std::vector<V3D> hkls =
          mg.getHKLs(std::min(0.7, static_cast<double>(i + 2)), 200.0,
                     CrystalStructure::UseStructureFactor);
      TS_ASSERT_DIFFERS(hkls.size(), 0);
    }
    time = t.elapsed();
    std::cout << time / 100.0 << std::endl;
  }

private:
  class HKLGenerator {
  public:
    HKLGenerator() {}
    ~HKLGenerator() {}

    std::vector<V3D> generateHKLs(const CrystalStructure &cs, double dMin,
                                  const HKLFilter &filter) const {
      std::vector<V3D> hkls;
      size_t estimatedReflectionCount =
          static_cast<size_t>(ceil(32.0 * M_PI * cs.cell().volume()) /
                              (3.0 * pow(2.0 * dMin, 3.0)));
      hkls.reserve(estimatedReflectionCount);

      int hMax = static_cast<int>(cs.cell().a() / dMin);
      int kMax = static_cast<int>(cs.cell().b() / dMin);
      int lMax = static_cast<int>(cs.cell().c() / dMin);

      for (int h = -hMax; h <= hMax; ++h) {
        for (int k = -kMax; k <= kMax; ++k) {
          for (int l = -lMax; l <= lMax; ++l) {
            V3D hkl(h, k, l);
            if (filter.isAllowed(hkl)) {
              hkls.push_back(hkl);
            }
          }
        }
      }

      return hkls;
    }
  };

  void inspectFilter(const HKLFilter &filter) {
    std::cout << filter.getName() << std::endl;

    try {
      const HKLFilterBinaryLogicOperation &op =
          dynamic_cast<const HKLFilterBinaryLogicOperation &>(filter);
      std::cout << "LHS: ";
      inspectFilter(op.getLHS());

      std::cout << "RHS: ";
      inspectFilter(op.getRHS());
    } catch(std::bad_cast) {
        // nothing.
    }
  }
};
#endif /* MANTID_GEOMETRY_HKLFILTERTEST_H_ */
