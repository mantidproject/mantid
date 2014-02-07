#ifndef POLDIDETECTORTEST_H
#define POLDIDETECTORTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/TableRow.h"
#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiHeliumDetector.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class TestablePoldiHeliumDetector : public Mantid::Poldi::PoldiHeliumDetector
{
    friend class PoldiDetectorTest;
};

class PoldiDetectorTest : public CxxTest::TestSuite
{
private:
    TableWorkspace_sptr m_configurationTestData;

public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiDetectorTest *createSuite() { return new PoldiDetectorTest(); }
    static void destroySuite( PoldiDetectorTest *suite ) { delete suite; }

    PoldiDetectorTest()
    {
        m_configurationTestData = TableWorkspace_sptr(new TableWorkspace(6));
        m_configurationTestData->addColumn(std::string("str"), std::string("name"));
        m_configurationTestData->addColumn(std::string("str"), std::string("unit"));
        m_configurationTestData->addColumn(std::string("double"), std::string("value"));

        TableRow radius(m_configurationTestData->getRow(0));
        radius << "det_radius" << "mm" << 3000.0;

        TableRow elementCount(m_configurationTestData->getRow(1));
        elementCount << "det_nb_channel" << "" << 400.0;

        TableRow elementWidth(m_configurationTestData->getRow(2));
        elementWidth << "det_channel_resolution" << "mm" << 2.5;

        TableRow x0det(m_configurationTestData->getRow(3));
        x0det << "x0det" << "mm" << -931.47;

        TableRow y0det(m_configurationTestData->getRow(4));
        y0det << "y0det" << "mm" << -860.0;

        TableRow twoTheta(m_configurationTestData->getRow(5));
        twoTheta << "twothet" << "" << 90.41;
    }

    void testDetectorInterface()
    {
        Mantid::Poldi::PoldiHeliumDetector *heliumDetector = new Mantid::Poldi::PoldiHeliumDetector();
        TS_ASSERT(heliumDetector);

        Mantid::Poldi::PoldiAbstractDetector *abstractDetector = static_cast<Mantid::Poldi::PoldiAbstractDetector *>(heliumDetector);
        TS_ASSERT(abstractDetector);

        Mantid::Poldi::PoldiHeliumDetector *reCastHeliumDetector = dynamic_cast<Mantid::Poldi::PoldiHeliumDetector *>(abstractDetector);
        TS_ASSERT(reCastHeliumDetector);

        delete heliumDetector;
    }

    void testConfigurationLoading()
    {
        Mantid::Poldi::PoldiHeliumDetector heliumDetector;
        TS_ASSERT_THROWS_NOTHING(heliumDetector.loadConfiguration(m_configurationTestData));

        for(size_t i = 0; i < m_configurationTestData->rowCount(); ++i) {
            TableWorkspace_sptr misConfigured(m_configurationTestData->clone());
            misConfigured->removeRow(i);

            TS_ASSERT_THROWS(heliumDetector.loadConfiguration(misConfigured), std::runtime_error);
        }
    }

    void testConfigurationCorrectness()
    {
        TestablePoldiHeliumDetector heliumDetector;
        heliumDetector.loadConfiguration(m_configurationTestData);

        TS_ASSERT_DELTA(heliumDetector.m_angularResolution, 0.0008333333333, 1e-6);
        TS_ASSERT_DELTA(heliumDetector.m_totalOpeningAngle, 0.3333333333333, 1e-6);
        TS_ASSERT_DELTA(heliumDetector.m_phiCenter, 1.260093451, 5e-7);
        TS_ASSERT_DELTA(heliumDetector.m_phiStart, 1.093426824, 5e-7);

        TS_ASSERT_EQUALS(heliumDetector.elementCount(), 400);
        TS_ASSERT_EQUALS(heliumDetector.centralElement(), 199);
    }

    void testPhi()
    {
        TestablePoldiHeliumDetector heliumDetector;
        heliumDetector.loadConfiguration(m_configurationTestData);

        TS_ASSERT_DELTA(heliumDetector.phi(199), 1.259676814, 5e-7);
    }

    void testTwoTheta()
    {
        Mantid::Poldi::PoldiHeliumDetector heliumDetector;
        heliumDetector.loadConfiguration(m_configurationTestData);

        TS_ASSERT_DELTA(heliumDetector.twoTheta(199), 1.577357650, 5e-7);
    }

    void testQLimits()
    {
        Mantid::Poldi::PoldiHeliumDetector heliumDetector;
        heliumDetector.loadConfiguration(m_configurationTestData);

        std::pair<double, double> qLimits = heliumDetector.qLimits(1.1, 5.0);

        TS_ASSERT_DELTA(qLimits.first, 1.549564, 1e-6);
        TS_ASSERT_DELTA(qLimits.second, 8.960878, 1e-6);
    }

    void testDistance()
    {
        Mantid::Poldi::PoldiHeliumDetector heliumDetector;
        heliumDetector.loadConfiguration(m_configurationTestData);

        TS_ASSERT_DELTA(heliumDetector.distanceFromSample(199), 1996.017578125, 1e-3);
    }

};

#endif // POLDIDETECTORTEST_H
