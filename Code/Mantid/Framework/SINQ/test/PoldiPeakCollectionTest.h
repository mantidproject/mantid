#ifndef MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H
#define MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/FlatBackground.h"

#include "MantidAPI/WorkspaceFactory.h"
#include <stdexcept>

using namespace Mantid::Poldi;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

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
        m_dummyData->addColumn("str", "FWHM");

        TableRow first = m_dummyData->appendRow();
        first << "1 0 0" << "0.5 +/- 0.001" << "12.566370 +/- 0.001000" << "2000 +/- 3" << "0.5 +/- 0.02";

        TableRow second = m_dummyData->appendRow();
        second << "1 1 0" << "0.8 +/- 0.004" << "7.853981 +/- 0.001000" << "200 +/- 14" << "0.9 +/- 0.1";

        m_dummyPeak = IPeakFunction_sptr(new Gaussian());
        m_dummyPeak->initialize();

        m_dummyBackground = IFunction_sptr(new FlatBackground());
        m_dummyBackground->initialize();
    }

    void testConstruction()
    {
        TS_ASSERT_THROWS_NOTHING(PoldiPeakCollection newCollection());

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

    void testSetProfileFunction()
    {
        TestablePoldiPeakCollection peaks(m_dummyData);

        peaks.setProfileFunction(m_dummyPeak);

        TS_ASSERT_EQUALS(peaks.m_profileTemplate, m_dummyPeak);
        TS_ASSERT_EQUALS(peaks.m_peakProfiles.size(), 2);

        peaks.addPeak(PoldiPeak::create(2.0));

        TS_ASSERT_EQUALS(peaks.m_peakProfiles.size(), 3);
    }

    void testSetBackgroundFunction()
    {
        TestablePoldiPeakCollection peaks(m_dummyData);

        peaks.setBackgroundFunction(m_dummyBackground);

        TS_ASSERT_EQUALS(peaks.m_backgroundTemplate, m_dummyBackground);
        TS_ASSERT_EQUALS(peaks.m_backgrounds.size(), 2);

        peaks.addPeak(PoldiPeak::create(2.0));

        TS_ASSERT_EQUALS(peaks.m_backgrounds.size(), 3);
    }

    void testSetTies()
    {
        TestablePoldiPeakCollection peaks(m_dummyData);

        peaks.setProfileTies("f1.A0 = f0.PeakCentre");

        TS_ASSERT_EQUALS(peaks.m_ties, "f1.A0 = f0.PeakCentre");
    }

    void testGetSinglePeakProfile()
    {
        PoldiPeakCollection peaks(m_dummyData);

        peaks.setProfileFunction(m_dummyPeak);

        PoldiPeak_sptr firstPeak = peaks.peak(0);
        IFunction_sptr firstProfile = peaks.getSinglePeakProfile(0);

        CompositeFunction_sptr profileComposite = boost::dynamic_pointer_cast<CompositeFunction>(firstProfile);
        TS_ASSERT(profileComposite);
        TS_ASSERT_EQUALS(profileComposite->nFunctions(), 1);

        IFunction_sptr firstCompositeMember = profileComposite->getFunction(0);
        IPeakFunction_sptr firstCompositePeak = boost::dynamic_pointer_cast<IPeakFunction>(firstCompositeMember);
        TS_ASSERT(firstCompositePeak);
        TS_ASSERT_EQUALS(firstCompositePeak->centre(), firstPeak->q());
        TS_ASSERT_EQUALS(firstCompositePeak->height(), firstPeak->intensity());
        TS_ASSERT_EQUALS(firstCompositePeak->fwhm(), firstPeak->fwhm());


        peaks.setBackgroundFunction(m_dummyBackground);
        CompositeFunction_sptr profileCompositeBg = boost::dynamic_pointer_cast<CompositeFunction>(peaks.getSinglePeakProfile(0));
        TS_ASSERT(profileCompositeBg);
        TS_ASSERT_EQUALS(profileCompositeBg->nFunctions(), 2);
        TS_ASSERT_EQUALS(profileCompositeBg->getFunction(1)->name(), "FlatBackground");
    }

    void testSetSingleProfileParameters()
    {
        PoldiPeakCollection peaks(m_dummyData);

        IPeakFunction_sptr peakProfile(new Gaussian());
        peakProfile->initialize();
        peakProfile->setCentre(3.0);
        peakProfile->setHeight(342.02);
        peakProfile->setFwhm(0.04223);

        CompositeFunction_sptr peakComposite(new CompositeFunction());
        peakComposite->addFunction(peakProfile);

        TS_ASSERT_THROWS_NOTHING(peaks.setSingleProfileParameters(0, peakComposite));

        PoldiPeak_sptr firstPeak = peaks.peak(0);
        TS_ASSERT_EQUALS(firstPeak->q(), 3.0);
        TS_ASSERT_EQUALS(firstPeak->intensity(), 342.02);
        TS_ASSERT_DELTA(firstPeak->fwhm(), 0.04223, 1e-9);
    }

    void testColumnCheckConsistency()
    {
        TestablePoldiPeakCollection peaks;

        TableWorkspace_sptr newTable(new TableWorkspace());
        peaks.prepareTable(newTable);

        TS_ASSERT(peaks.checkColumns(newTable));
    }


private:
    TableWorkspace_sptr m_dummyData;
    IPeakFunction_sptr m_dummyPeak;
    IFunction_sptr m_dummyBackground;
};

#endif // MANTID_SINQ_POLDIPEAKCOLLECTIONTEST_H
