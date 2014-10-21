#ifndef MANTID_GEOMETRY_SCATTERERCOLLECTIONTEST_H_
#define MANTID_GEOMETRY_SCATTERERCOLLECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/ScattererCollection.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"
#include <map>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;


class ScattererCollectionTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static ScattererCollectionTest *createSuite() { return new ScattererCollectionTest(); }
    static void destroySuite( ScattererCollectionTest *suite ) { delete suite; }


    void testConstructor()
    {
        TS_ASSERT_THROWS_NOTHING(ScattererCollection scatterers);
    }

    void testCreate()
    {
        TS_ASSERT_THROWS_NOTHING(ScattererCollection_sptr scatterer = ScattererCollection::create());

        std::vector<IScatterer_sptr> scatterers;
        scatterers.push_back(IsotropicAtomScatterer::create("Si", V3D(0.35, 0, 0)));
        scatterers.push_back(IsotropicAtomScatterer::create("Si", V3D(0.25, 0.25, 0.25)));

        ScattererCollection_sptr scatterer = ScattererCollection::create(scatterers);
        TS_ASSERT_EQUALS(scatterer->nScatterers(), 2);
        TS_ASSERT_EQUALS(scatterer->getScatterer(0)->getPosition(), V3D(0.35, 0, 0));
        TS_ASSERT_EQUALS(scatterer->getScatterer(1)->getPosition(), V3D(0.25, 0.25, 0.25));
    }

    void testClone()
    {
        ScattererCollection_sptr scatterer = getScattererCollection();

        IScatterer_sptr clone = scatterer->clone();
        ScattererCollection_sptr collectionClone = boost::dynamic_pointer_cast<ScattererCollection>(clone);

        TS_ASSERT(collectionClone);
        TS_ASSERT_EQUALS(collectionClone->nScatterers(), 2);
        TS_ASSERT_EQUALS(collectionClone->getScatterer(0)->getPosition(), V3D(0.35, 0, 0));
        TS_ASSERT_EQUALS(collectionClone->getScatterer(1)->getPosition(), V3D(0.25, 0.25, 0.25));
    }

    void testSetCell()
    {
        ScattererCollection_sptr scatterer = getScattererCollection();

        UnitCell cell(5.43, 5.43, 5.43);
        TS_ASSERT_DIFFERS(scatterer->getScatterer(0)->getCell().getG(), cell.getG());

        scatterer->setCell(cell);
        TS_ASSERT_EQUALS(scatterer->getScatterer(0)->getCell().getG(), cell.getG());
        TS_ASSERT_EQUALS(scatterer->getScatterer(1)->getCell().getG(), cell.getG());
    }

    void testSetSpaceGroup()
    {
        ScattererCollection_sptr scatterer = getScattererCollection();

        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");
        TS_ASSERT(spaceGroup);

        TS_ASSERT_DIFFERS(scatterer->getScatterer(0)->getSpaceGroup(), spaceGroup);

        scatterer->setSpaceGroup(spaceGroup);
        TS_ASSERT_EQUALS(scatterer->getSpaceGroup(), spaceGroup);
        TS_ASSERT_EQUALS(scatterer->getScatterer(0)->getSpaceGroup(), spaceGroup);
        TS_ASSERT_EQUALS(scatterer->getScatterer(1)->getSpaceGroup(), spaceGroup);
    }

    void testAddGetScatterer()
    {
        UnitCell cell(5.43, 5.43, 5.43);
        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");

        ScattererCollection_sptr scatterer = ScattererCollection::create();
        scatterer->setCell(cell);
        scatterer->setSpaceGroup(spaceGroup);

        IsotropicAtomScatterer_sptr siOne = IsotropicAtomScatterer::create("Si", V3D(0, 0, 0));
        TS_ASSERT_DIFFERS(siOne->getSpaceGroup(), spaceGroup);

        size_t oldCount = scatterer->nScatterers();
        scatterer->addScatterer(siOne);
        TS_ASSERT_EQUALS(scatterer->nScatterers(), oldCount + 1);

        // The scatterer is cloned, so the new space group is not in siOne
        TS_ASSERT_EQUALS(scatterer->getScatterer(0)->getSpaceGroup(), spaceGroup);
        TS_ASSERT_DIFFERS(siOne->getSpaceGroup(), spaceGroup);

        TS_ASSERT_THROWS(scatterer->getScatterer(2), std::out_of_range);
    }

    void testRemoveScatterer()
    {
        ScattererCollection_sptr scattererCollection = getScattererCollection();
        size_t oldCount = scattererCollection->nScatterers();

        TS_ASSERT_THROWS_NOTHING(scattererCollection->getScatterer(oldCount - 1));
        TS_ASSERT_THROWS_NOTHING(scattererCollection->removeScatterer(0));

        TS_ASSERT_EQUALS(scattererCollection->nScatterers(), oldCount - 1);

        TS_ASSERT_THROWS(scattererCollection->getScatterer(oldCount - 1), std::out_of_range);
        TS_ASSERT_THROWS(scattererCollection->removeScatterer(10), std::out_of_range);
    }

    void testStructureFactorCalculation()
    {
        /* To check that structure factor calculation is correct also for
         * oblique cells with low symmetry, this hypothetical Si with monoclinic
         * cell and one atom in a general position is used.
         *
         * For comparison, a shelxl .ins file was prepared with the structure and
         * squared structure factor amplitudes were calculated using the LIST 4 option.
         */
        UnitCell cell(5.43, 6.43, 7.43, 90.0, 103.0, 90.0);
        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");

        ScattererCollection_sptr coll = boost::make_shared<ScattererCollection>();
        coll->setSpaceGroup(spaceGroup);
        coll->setCell(cell);

        coll->addScatterer(boost::make_shared<IsotropicAtomScatterer>("Si", V3D(0.2, 0.3, 0.4), 0.01267));

        // Load reference data, obtained with SHELXL-2014.
        std::map<V3D, double> referenceData = getCalculatedStructureFactors();

        for(auto it = referenceData.begin(); it != referenceData.end(); ++it) {
            double ampl = std::abs(coll->calculateStructureFactor(it->first));
            double sqAmpl = ampl * ampl;

            TS_ASSERT_DELTA(sqAmpl, it->second, 6e-3);
        }
    }

private:
    ScattererCollection_sptr getScattererCollection()
    {
        std::vector<IScatterer_sptr> scatterers;
        scatterers.push_back(IsotropicAtomScatterer::create("Si", V3D(0.35, 0, 0)));
        scatterers.push_back(IsotropicAtomScatterer::create("Si", V3D(0.25, 0.25, 0.25)));

        return ScattererCollection::create(scatterers);
    }

    std::map<V3D, double> getCalculatedStructureFactors()
    {
        std::map<V3D, double> fSquaredCalc;
        fSquaredCalc.insert(std::make_pair(V3D(2, 0, 0),  167.84));
        fSquaredCalc.insert(std::make_pair(V3D(3, 0, 0),  153.50));
        fSquaredCalc.insert(std::make_pair(V3D(4, 0, 0),  19.76));
        fSquaredCalc.insert(std::make_pair(V3D(5, 0, 0),  176.21));
        fSquaredCalc.insert(std::make_pair(V3D(1, 1, 0),  2.44));
        fSquaredCalc.insert(std::make_pair(V3D(2, 1, 0),  15.83));
        fSquaredCalc.insert(std::make_pair(V3D(3, 1, 0),  14.48));
        fSquaredCalc.insert(std::make_pair(V3D(4, 1, 0),  1.86));
        fSquaredCalc.insert(std::make_pair(V3D(5, 1, 0),  16.62));
        fSquaredCalc.insert(std::make_pair(V3D(2, 2, 0),  104.66));
        fSquaredCalc.insert(std::make_pair(V3D(3, 2, 0),  95.72));
        fSquaredCalc.insert(std::make_pair(V3D(4, 2, 0),  12.32));
        fSquaredCalc.insert(std::make_pair(V3D(5, 2, 0),  109.88));
        fSquaredCalc.insert(std::make_pair(V3D(3, 3, 0),  90.10));
        fSquaredCalc.insert(std::make_pair(V3D(4, 3, 0),  11.60));
        fSquaredCalc.insert(std::make_pair(V3D(5, 3, 0),  103.43));
        fSquaredCalc.insert(std::make_pair(V3D(4, 4, 0),  1.55));
        fSquaredCalc.insert(std::make_pair(V3D(5, 4, 0),  13.86));
        fSquaredCalc.insert(std::make_pair(V3D(5, 5, 0),  130.22));
        fSquaredCalc.insert(std::make_pair(V3D(1, 1, 1),  16.45));
        fSquaredCalc.insert(std::make_pair(V3D(2, 1, 1),  2.26));
        fSquaredCalc.insert(std::make_pair(V3D(3, 1, 1),  21.53));
        fSquaredCalc.insert(std::make_pair(V3D(4, 1, 1),  1.80));
        fSquaredCalc.insert(std::make_pair(V3D(5, 1, 1),  10.47));
        fSquaredCalc.insert(std::make_pair(V3D(2, 2, 1),  14.95));
        fSquaredCalc.insert(std::make_pair(V3D(3, 2, 1),  142.33));
        fSquaredCalc.insert(std::make_pair(V3D(4, 2, 1),  11.92));
        fSquaredCalc.insert(std::make_pair(V3D(5, 2, 1),  69.17));
        fSquaredCalc.insert(std::make_pair(V3D(3, 3, 1),  133.97));
        fSquaredCalc.insert(std::make_pair(V3D(4, 3, 1),  11.22));
        fSquaredCalc.insert(std::make_pair(V3D(5, 3, 1),  65.11));
        fSquaredCalc.insert(std::make_pair(V3D(4, 4, 1),  1.50));
        fSquaredCalc.insert(std::make_pair(V3D(5, 4, 1),  8.73));
        fSquaredCalc.insert(std::make_pair(V3D(5, 5, 1),  81.98));
        fSquaredCalc.insert(std::make_pair(V3D(2, 2, 2),  14.36));
        fSquaredCalc.insert(std::make_pair(V3D(3, 2, 2),  88.94));
        fSquaredCalc.insert(std::make_pair(V3D(4, 2, 2),  77.57));
        fSquaredCalc.insert(std::make_pair(V3D(5, 2, 2),  9.52));
        fSquaredCalc.insert(std::make_pair(V3D(3, 3, 2),  83.72));
        fSquaredCalc.insert(std::make_pair(V3D(4, 3, 2),  73.02));
        fSquaredCalc.insert(std::make_pair(V3D(5, 3, 2),  8.96));
        fSquaredCalc.insert(std::make_pair(V3D(4, 4, 2),  9.79));
        fSquaredCalc.insert(std::make_pair(V3D(5, 4, 2),  1.20));
        fSquaredCalc.insert(std::make_pair(V3D(5, 5, 2),  11.29));
        fSquaredCalc.insert(std::make_pair(V3D(3, 3, 3),  11.44));
        fSquaredCalc.insert(std::make_pair(V3D(4, 3, 3),  103.89));
        fSquaredCalc.insert(std::make_pair(V3D(5, 3, 3),  8.30));
        fSquaredCalc.insert(std::make_pair(V3D(4, 4, 3),  13.93));
        fSquaredCalc.insert(std::make_pair(V3D(5, 4, 3),  1.11));
        fSquaredCalc.insert(std::make_pair(V3D(5, 5, 3),  10.45));
        fSquaredCalc.insert(std::make_pair(V3D(4, 4, 4),  8.33));
        fSquaredCalc.insert(std::make_pair(V3D(5, 4, 4),  6.93));
        fSquaredCalc.insert(std::make_pair(V3D(5, 5, 4),  65.05));
        fSquaredCalc.insert(std::make_pair(V3D(5, 5, 5),  88.57));
        return fSquaredCalc;
    }
};


#endif /* MANTID_GEOMETRY_SCATTERERCOLLECTIONTEST_H_ */
