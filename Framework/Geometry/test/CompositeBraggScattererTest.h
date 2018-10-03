// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_BRAGGSCATTERERCOLLECTIONTEST_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERCOLLECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/V3D.h"

#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include <iostream>
#include <map>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CompositeBraggScattererTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompositeBraggScattererTest *createSuite() {
    return new CompositeBraggScattererTest();
  }
  static void destroySuite(CompositeBraggScattererTest *suite) { delete suite; }

  void testConstructor() {
    TS_ASSERT_THROWS_NOTHING(CompositeBraggScatterer scatterers);
  }

  void testCreate() {
    TS_ASSERT_THROWS_NOTHING(CompositeBraggScatterer_sptr scatterer =
                                 CompositeBraggScatterer::create());

    std::vector<BraggScatterer_sptr> scatterers{
        getInitializedScatterer("Si", "0.35, 0, 0"),
        getInitializedScatterer("Si", "1/4, 1/4, 1/4")};

    CompositeBraggScatterer_sptr scatterer =
        CompositeBraggScatterer::create(scatterers);
    TS_ASSERT_EQUALS(scatterer->nScatterers(), 2);
    TS_ASSERT_EQUALS(
        boost::dynamic_pointer_cast<BraggScattererInCrystalStructure>(
            scatterer->getScatterer(0))
            ->getPosition(),
        V3D(0.35, 0, 0));
    TS_ASSERT_EQUALS(
        boost::dynamic_pointer_cast<BraggScattererInCrystalStructure>(
            scatterer->getScatterer(1))
            ->getPosition(),
        V3D(0.25, 0.25, 0.25));
  }

  void testClone() {
    CompositeBraggScatterer_sptr scatterer = getCompositeScatterer();
    BraggScatterer_sptr clone = scatterer->clone();

    CompositeBraggScatterer_sptr collectionClone =
        boost::dynamic_pointer_cast<CompositeBraggScatterer>(clone);

    TS_ASSERT(collectionClone);
    TS_ASSERT_EQUALS(collectionClone->nScatterers(), 2);
    TS_ASSERT_EQUALS(
        boost::dynamic_pointer_cast<BraggScattererInCrystalStructure>(
            collectionClone->getScatterer(0))
            ->getPosition(),
        V3D(0.35, 0, 0));
    TS_ASSERT_EQUALS(
        boost::dynamic_pointer_cast<BraggScattererInCrystalStructure>(
            collectionClone->getScatterer(1))
            ->getPosition(),
        V3D(0.25, 0.25, 0.25));
  }

  void testAddGetScatterer() {
    CompositeBraggScatterer_sptr scatterer = CompositeBraggScatterer::create();
    TS_ASSERT_EQUALS(scatterer->propertyCount(), 0);

    IsotropicAtomBraggScatterer_sptr siOne =
        getInitializedScatterer("Si", "[0, 0, 0]");
    size_t oldCount = scatterer->nScatterers();
    scatterer->addScatterer(siOne);
    TS_ASSERT_EQUALS(scatterer->propertyCount(), 1);

    TS_ASSERT_EQUALS(scatterer->nScatterers(), oldCount + 1);
    TS_ASSERT_THROWS(scatterer->getScatterer(2), std::out_of_range);
  }

  void testRemoveScatterer() {
    CompositeBraggScatterer_sptr scattererCollection = getCompositeScatterer();
    size_t oldCount = scattererCollection->nScatterers();

    TS_ASSERT_THROWS_NOTHING(scattererCollection->getScatterer(oldCount - 1));
    TS_ASSERT_THROWS_NOTHING(scattererCollection->removeScatterer(0));

    TS_ASSERT_EQUALS(scattererCollection->nScatterers(), oldCount - 1);

    TS_ASSERT_THROWS(scattererCollection->getScatterer(oldCount - 1),
                     std::out_of_range);
    TS_ASSERT_THROWS(scattererCollection->removeScatterer(10),
                     std::out_of_range);

    scattererCollection->removeScatterer(0);

    // Unused properties are removed, so when there are no scatterers, there are
    // no properties.
    TS_ASSERT_EQUALS(scattererCollection->propertyCount(), 0);
  }

  void testRemoveAllScatterers() {
    CompositeBraggScatterer_sptr scattererCollection = getCompositeScatterer();

    TS_ASSERT_DIFFERS(scattererCollection->nScatterers(), 0);
    TS_ASSERT_THROWS_NOTHING(scattererCollection->removeAllScatterers());
    TS_ASSERT_EQUALS(scattererCollection->nScatterers(), 0);

    TS_ASSERT_THROWS_NOTHING(scattererCollection->removeAllScatterers());
    TS_ASSERT_EQUALS(scattererCollection->propertyCount(), 0);
  }

  void testStructureFactorCalculation() {
    /* To check that structure factor calculation is correct also for
     * oblique cells with low symmetry, this hypothetical Si with monoclinic
     * cell and one atom in a general position is used.
     *
     * For comparison, a shelxl .ins file was prepared with the structure and
     * squared structure factor amplitudes were calculated using the LIST 4
     *option.
     */
    UnitCell cell(5.43, 6.43, 7.43, 90.0, 103.0, 90.0);
    SpaceGroup_const_sptr spaceGroup =
        SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");
    std::vector<V3D> positions =
        spaceGroup->getEquivalentPositions(V3D(0.2, 0.3, 0.4));

    CompositeBraggScatterer_sptr coll = CompositeBraggScatterer::create();
    for (auto &position : positions) {
      std::ostringstream strm;
      strm << position;
      coll->addScatterer(getInitializedScatterer("Si", strm.str(), 0.01267));
    }

    coll->setProperty("UnitCell", unitCellToStr(cell));

    // Load reference data, obtained with SHELXL-2014.
    std::map<V3D, double> referenceData = getCalculatedStructureFactors();

    for (auto it = referenceData.begin(); it != referenceData.end(); ++it) {
      double ampl = std::abs(coll->calculateStructureFactor(it->first));
      double sqAmpl = ampl * ampl;

      // F^2 is calculated to two decimal places, so the maximum deviation is
      // 5e-3,
      TS_ASSERT_DELTA(sqAmpl, it->second, 5.1e-3);
    }
  }

private:
  IsotropicAtomBraggScatterer_sptr
  getInitializedScatterer(const std::string &element,
                          const std::string &position, double U = 0.0,
                          double occ = 1.0) {
    IsotropicAtomBraggScatterer_sptr scatterer =
        boost::make_shared<IsotropicAtomBraggScatterer>();
    scatterer->initialize();
    scatterer->setProperty("Element", element);
    scatterer->setProperty("Position", position);
    scatterer->setProperty("U", U);
    scatterer->setProperty("Occupancy", occ);

    return scatterer;
  }

  CompositeBraggScatterer_sptr getCompositeScatterer() {
    return CompositeBraggScatterer::create(
        {getInitializedScatterer("Si", "[0.35, 0, 0]"),
         getInitializedScatterer("Si", "1/4, 1/4, 1/4")});
  }

  std::map<V3D, double> getCalculatedStructureFactors() {
    return {{{2, 0, 0}, 167.84}, {{3, 0, 0}, 153.50}, {{4, 0, 0}, 19.76},
            {{5, 0, 0}, 176.21}, {{1, 1, 0}, 2.44},   {{2, 1, 0}, 15.83},
            {{3, 1, 0}, 14.48},  {{4, 1, 0}, 1.86},   {{5, 1, 0}, 16.62},
            {{2, 2, 0}, 104.66}, {{3, 2, 0}, 95.72},  {{4, 2, 0}, 12.32},
            {{5, 2, 0}, 109.88}, {{3, 3, 0}, 90.10},  {{4, 3, 0}, 11.60},
            {{5, 3, 0}, 103.43}, {{4, 4, 0}, 1.55},   {{5, 4, 0}, 13.86},
            {{5, 5, 0}, 130.22}, {{1, 1, 1}, 16.45},  {{2, 1, 1}, 2.26},
            {{3, 1, 1}, 21.53},  {{4, 1, 1}, 1.80},   {{5, 1, 1}, 10.47},
            {{2, 2, 1}, 14.95},  {{3, 2, 1}, 142.33}, {{4, 2, 1}, 11.92},
            {{5, 2, 1}, 69.17},  {{3, 3, 1}, 133.97}, {{4, 3, 1}, 11.22},
            {{5, 3, 1}, 65.11},  {{4, 4, 1}, 1.50},   {{5, 4, 1}, 8.73},
            {{5, 5, 1}, 81.98},  {{2, 2, 2}, 14.36},  {{3, 2, 2}, 88.94},
            {{4, 2, 2}, 77.57},  {{5, 2, 2}, 9.52},   {{3, 3, 2}, 83.72},
            {{4, 3, 2}, 73.02},  {{5, 3, 2}, 8.96},   {{4, 4, 2}, 9.79},
            {{5, 4, 2}, 1.20},   {{5, 5, 2}, 11.29},  {{3, 3, 3}, 11.44},
            {{4, 3, 3}, 103.89}, {{5, 3, 3}, 8.30},   {{4, 4, 3}, 13.93},
            {{5, 4, 3}, 1.11},   {{5, 5, 3}, 10.45},  {{4, 4, 4}, 8.33},
            {{5, 4, 4}, 6.93},   {{5, 5, 4}, 65.05},  {{5, 5, 5}, 88.57}};
  }
};

#endif /* MANTID_GEOMETRY_BRAGGSCATTERERCOLLECTIONTEST_H_ */
