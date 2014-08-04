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

    void testColumnCheckConsistency()
    {
        TestablePoldiPeakCollection peaks;

        TableWorkspace_sptr newTable(new TableWorkspace());
        peaks.prepareTable(newTable);

        TS_ASSERT(peaks.checkColumns(newTable));
    }

    void testGetUniqueHKLSet()
    {
        // a cubic primitive cell
        UnitCell CsCl(4.126, 4.126, 4.126);
        PointGroup_sptr m3m = boost::make_shared<PointGroupLaue13>();
        double dMin = 0.55;
        double dMax = 5.0;

        TestablePoldiPeakCollection p;

        std::vector<V3D> peaks = p.getUniqueHKLSet(CsCl, m3m, dMin, dMax);

        TS_ASSERT_EQUALS(peaks.size(), 69);
        TS_ASSERT_EQUALS(peaks[0], V3D(1, 0, 0));
        TS_ASSERT_EQUALS(peaks[12], V3D(3, 2, 0));
        TS_ASSERT_EQUALS(peaks[68], V3D(7, 2, 1));
    }

    void testSetPeaks()
    {
        UnitCell CsCl(4.126, 4.126, 4.126);
        PointGroup_sptr m3m = boost::make_shared<PointGroupLaue13>();
        double dMin = 0.55;
        double dMax = 5.0;

        TestablePoldiPeakCollection p;

        std::vector<V3D> peaks = p.getUniqueHKLSet(CsCl, m3m, dMin, dMax);
        p.setPeaks(peaks, CsCl);

        TS_ASSERT_EQUALS(p.peakCount(), 69);

        PoldiPeak_sptr peak1 = p.peak(0);
        TS_ASSERT_EQUALS(peak1->hkl(), MillerIndices(1, 0, 0));
        TS_ASSERT_EQUALS(peak1->d(), 4.126);

        PoldiPeak_sptr peak68 = p.peak(68);
        TS_ASSERT_EQUALS(peak68->hkl(), MillerIndices(7, 2, 1));
        TS_ASSERT_DELTA(peak68->d(), 0.5615, 1e-4)
    }


private:
    TableWorkspace_sptr m_dummyData;
};

#endif // MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H
