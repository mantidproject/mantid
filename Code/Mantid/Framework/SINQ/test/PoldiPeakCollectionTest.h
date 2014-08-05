#ifndef MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H
#define MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include "MantidAPI/WorkspaceFactory.h"
#include <stdexcept>

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class PoldiPeakCollectionTest;

class TestablePoldiPeakCollection : public PoldiPeakCollection
{
    friend class PoldiPeakCollectionTest;

    TestablePoldiPeakCollection() :
        PoldiPeakCollection()
    {
    }

    TestablePoldiPeakCollection(TableWorkspace_sptr workspace) :
        PoldiPeakCollection(workspace)
    {
    }
};

class PoldiPeakCollectionTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiPeakCollectionTest *createSuite() { return new PoldiPeakCollectionTest(); }
    static void destroySuite( PoldiPeakCollectionTest *suite ) { delete suite; }

    PoldiPeakCollectionTest()
    {
        m_dummyData = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());
        m_dummyData->addColumn("str", "HKL");
        m_dummyData->addColumn("str", "d");
        m_dummyData->addColumn("str", "Q");
        m_dummyData->addColumn("str", "Intensity");
        m_dummyData->addColumn("str", "FWHM (rel.)");

        TableRow first = m_dummyData->appendRow();
        first << "1 0 0" << "0.5 +/- 0.001" << "12.566370 +/- 0.001000" << "2000 +/- 3" << "0.5 +/- 0.02";

        TableRow second = m_dummyData->appendRow();
        second << "1 1 0" << "0.8 +/- 0.004" << "7.853981 +/- 0.001000" << "200 +/- 14" << "0.9 +/- 0.1";
    }

    void testConstruction()
    {
        TS_ASSERT_THROWS_NOTHING(PoldiPeakCollection newCollection);

        TS_ASSERT_THROWS_NOTHING(PoldiPeakCollection fromTable(m_dummyData));
    }

    void testTableImportExport()
    {
        PoldiPeakCollection fromTable(m_dummyData);
        TS_ASSERT_EQUALS(fromTable.peakCount(), 2);

        PoldiPeak_sptr first = fromTable.peak(0);
        TS_ASSERT_EQUALS(first->d(), 0.5);
        TS_ASSERT_EQUALS(first->d().error(), 0.001);
        TS_ASSERT_EQUALS(first->q(), 2.0 * M_PI / 0.5);
        TS_ASSERT_EQUALS(first->fwhm(PoldiPeak::Relative), 0.5);
        TS_ASSERT_EQUALS(first->fwhm(PoldiPeak::AbsoluteD), 0.25);

        TableWorkspace_sptr exported = fromTable.asTableWorkspace();
        TS_ASSERT_EQUALS(exported->columnCount(), 5);
        TS_ASSERT_EQUALS(exported->rowCount(), 2);

        TableRow secondRowReference = m_dummyData->getRow(1);
        TableRow secondRow = exported->getRow(1);

        // HKL strings compare directly
        TS_ASSERT_EQUALS(secondRow.cell<std::string>(0), secondRowReference.cell<std::string>(0));

        // The other values not necessarily (string conversion of UncertainValue)
        for(size_t i = 1; i < exported->columnCount(); ++i) {
            TS_ASSERT_DELTA(UncertainValueIO::fromString(secondRow.cell<std::string>(i)).value(), UncertainValueIO::fromString(secondRowReference.cell<std::string>(i)).value(), 1e-6);
        }
    }

    void testProfileFunctionName()
    {
        TestablePoldiPeakCollection collection;

        TS_ASSERT(collection.m_profileFunctionName.empty());

        collection.setProfileFunctionName("Gaussian");

        TS_ASSERT_EQUALS(collection.m_profileFunctionName, "Gaussian");
    }

    void testProfileFunctionRevovery()
    {
        TestablePoldiPeakCollection collection;
        collection.setProfileFunctionName("Gaussian");

        TableWorkspace_sptr table = collection.asTableWorkspace();

        TestablePoldiPeakCollection other(table);

        TS_ASSERT_EQUALS(other.getProfileFunctionName(), "Gaussian");
        TS_ASSERT(other.hasProfileFunctionName());
    }

    void testMissingProfileFunction()
    {
        TestablePoldiPeakCollection collection(m_dummyData);
        TS_ASSERT(!collection.hasProfileFunctionName());
        TS_ASSERT(collection.getProfileFunctionName().empty());
    }


    void testIntensityTypeFromString()
    {
        TestablePoldiPeakCollection collection;

        TS_ASSERT_EQUALS(collection.intensityTypeFromString("Maximum"), PoldiPeakCollection::Maximum);
        TS_ASSERT_EQUALS(collection.intensityTypeFromString("maximum"), PoldiPeakCollection::Maximum);
        TS_ASSERT_EQUALS(collection.intensityTypeFromString("mAxIMuM"), PoldiPeakCollection::Maximum);

        TS_ASSERT_EQUALS(collection.intensityTypeFromString("Integral"), PoldiPeakCollection::Integral);
        TS_ASSERT_EQUALS(collection.intensityTypeFromString("integral"), PoldiPeakCollection::Integral);
        TS_ASSERT_EQUALS(collection.intensityTypeFromString("InTEgrAl"), PoldiPeakCollection::Integral);

        TS_ASSERT_EQUALS(collection.intensityTypeFromString("Garbage"), PoldiPeakCollection::Maximum);
        TS_ASSERT_EQUALS(collection.intensityTypeFromString(""), PoldiPeakCollection::Maximum);
    }

    void testIntensityTypeToString()
    {
        TestablePoldiPeakCollection collection;
        TS_ASSERT_EQUALS(collection.intensityTypeToString(PoldiPeakCollection::Maximum), "Maximum");
        TS_ASSERT_EQUALS(collection.intensityTypeToString(PoldiPeakCollection::Integral), "Integral");
    }

    void testIntensityTypeRecovery()
    {
        PoldiPeakCollection collection(m_dummyData);

        TS_ASSERT_EQUALS(collection.intensityType(), PoldiPeakCollection::Maximum);

        TableWorkspace_sptr newDummy(m_dummyData->clone());
        newDummy->logs()->addProperty<std::string>("IntensityType", "Integral");

        PoldiPeakCollection otherCollection(newDummy);
        TS_ASSERT_EQUALS(otherCollection.intensityType(), PoldiPeakCollection::Integral);
    }

    void testIntensityTypeRecoveryConversion()
    {
        TableWorkspace_sptr newDummy(m_dummyData->clone());
        newDummy->logs()->addProperty<std::string>("IntensityType", "Integral");

        PoldiPeakCollection collection(newDummy);

        TableWorkspace_sptr compare = collection.asTableWorkspace();

        TS_ASSERT(compare->logs()->hasProperty("IntensityType"));
        TS_ASSERT_EQUALS(compare->logs()->getPropertyValueAsType<std::string>("IntensityType"), "Integral");

        PoldiPeakCollection otherCollection(compare);

        TS_ASSERT_EQUALS(otherCollection.intensityType(), PoldiPeakCollection::Integral);
    }

    void testAddPeak()
    {
        PoldiPeakCollection peaks;
        peaks.addPeak(PoldiPeak::create(2.0));

        TS_ASSERT_EQUALS(peaks.peakCount(), 1);
    }

    void testPeakAccess()
    {
        PoldiPeakCollection peaks;
        PoldiPeak_sptr newPeak = PoldiPeak::create(2.0);
        peaks.addPeak(newPeak);

        PoldiPeak_sptr peak = peaks.peak(0);
        TS_ASSERT_EQUALS(peak, newPeak);

        TS_ASSERT_THROWS(peaks.peak(1), std::range_error);
        TS_ASSERT_THROWS(peaks.peak(-2), std::range_error);
    }

    void testPeaksVector()
    {
        PoldiPeakCollection fromTable(m_dummyData);
        std::vector<PoldiPeak_sptr> peaks = fromTable.peaks();

        // make sure that a copy of the vector is created
        peaks.clear();
        TS_ASSERT_EQUALS(fromTable.peakCount(), 2);

    }

    void testColumnCheckConsistency()
    {
        TestablePoldiPeakCollection peaks;

        TableWorkspace_sptr newTable(new TableWorkspace());
        peaks.prepareTable(newTable);

        TS_ASSERT(peaks.checkColumns(newTable));
    }

    void testClone()
    {
        PoldiPeakCollection_sptr peaks(new PoldiPeakCollection);
        peaks->setProfileFunctionName("Test");
        peaks->addPeak(PoldiPeak::create(2.0));
        peaks->addPeak(PoldiPeak::create(3.0));

        PoldiPeakCollection_sptr clone = peaks->clone();

        // make sure those are different instances
        TS_ASSERT(clone != peaks);

        // everything else should be identical
        TS_ASSERT_EQUALS(clone->getProfileFunctionName(), peaks->getProfileFunctionName());
        TS_ASSERT_EQUALS(clone->intensityType(), peaks->intensityType());
        TS_ASSERT_EQUALS(clone->peakCount(), peaks->peakCount());

        for(size_t i = 0; i < clone->peakCount(); ++i) {
            PoldiPeak_sptr clonePeak = clone->peak(i);
            PoldiPeak_sptr peaksPeak = peaks->peak(i);

            TS_ASSERT(clonePeak != peaksPeak);
            TS_ASSERT_EQUALS(clonePeak->d(), peaksPeak->d());
        }
    }

    void testStructureConstructor()
    {
        UnitCell CsCl(4.126, 4.126, 4.126);
        PointGroup_sptr m3m = boost::make_shared<PointGroupLaue13>();

        CrystalStructure_sptr structure = boost::make_shared<CrystalStructure>(CsCl, m3m);

        double dMin = 0.55;
        double dMax = 5.0;

        /* The peak collection should contain all allowed symmetry independent HKLs
         * between 0.55 and 5.0 Angstrom, for the unit cell of CsCl
         * (Primitive cubic cell with a = 4.126 Angstrom, Pointgroup m-3m).
         */
        PoldiPeakCollection p(structure, dMin, dMax);

        TS_ASSERT_EQUALS(p.peakCount(), 69);

        PoldiPeak_sptr peak1 = p.peak(0);
        TS_ASSERT_EQUALS(peak1->hkl(), MillerIndices(1, 0, 0));
        TS_ASSERT_EQUALS(peak1->d(), 4.126);

        PoldiPeak_sptr peak68 = p.peak(68);
        TS_ASSERT_EQUALS(peak68->hkl(), MillerIndices(7, 2, 1));
        TS_ASSERT_DELTA(peak68->d(), 0.5615, 1e-4);

        std::vector<PoldiPeak_sptr> poldiPeaks = p.peaks();

        // sort peak list and check that all peaks are within the limits
        std::sort(poldiPeaks.begin(), poldiPeaks.end(), boost::bind<bool>(&PoldiPeak::greaterThan, _1, _2, &PoldiPeak::d));

        TS_ASSERT_LESS_THAN_EQUALS(poldiPeaks[0]->d(), 5.0);
        TS_ASSERT_LESS_THAN_EQUALS(0.55, poldiPeaks[68]->d());
        TS_ASSERT_LESS_THAN(poldiPeaks[68]->d(), poldiPeaks[0]->d());
    }

    void testSetPeaks()
    {
        UnitCell CsCl(4.126, 4.126, 4.126);
        PointGroup_sptr m3m = boost::make_shared<PointGroupLaue13>();

        CrystalStructure_sptr structure = boost::make_shared<CrystalStructure>(CsCl, m3m);

        double dMin = 0.55;
        double dMax = 5.0;

        std::vector<V3D> hkls = structure->getUniqueHKLs(dMin, dMax);
        std::vector<double> dValues = structure->getDValues(hkls);

        TestablePoldiPeakCollection p;
        TS_ASSERT_THROWS_NOTHING(p.setPeaks(hkls, dValues));

        dValues.pop_back();
        TS_ASSERT_THROWS(p.setPeaks(hkls, dValues), std::invalid_argument);
    }

private:
    TableWorkspace_sptr m_dummyData;
};

#endif // MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H
