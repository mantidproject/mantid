#ifndef MANTID_SINQ_POLDIBASICCHOPPERTEST_H_
#define MANTID_SINQ_POLDIBASICCHOPPERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/TableRow.h"
#include "MantidSINQ/PoldiBasicChopper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using Mantid::Poldi::PoldiBasicChopper;

class PoldiBasicChopperTest : public CxxTest::TestSuite
{
private:
    TableWorkspace_sptr m_chopperConfigurationWorkspace;
    TableWorkspace_sptr m_chopperSlitWorkspace;
    TableWorkspace_sptr m_rotationSpeedWorkspace;
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiBasicChopperTest *createSuite() { return new PoldiBasicChopperTest(); }
    static void destroySuite( PoldiBasicChopperTest *suite ) { delete suite; }

    PoldiBasicChopperTest()
    {
        m_chopperConfigurationWorkspace = TableWorkspace_sptr(new TableWorkspace(3));
        m_chopperConfigurationWorkspace->addColumn(std::string("str"), std::string("name"));
        m_chopperConfigurationWorkspace->addColumn(std::string("str"), std::string("unit"));
        m_chopperConfigurationWorkspace->addColumn(std::string("double"), std::string("value"));

        TableRow chopperDistance(m_chopperConfigurationWorkspace->getRow(0));
        chopperDistance << "dist_chopper_sample" << "mm" << 11800.0;

        TableRow t0(m_chopperConfigurationWorkspace->getRow(1));
        t0 << "t0" << "mysec" << 0.0005;

        TableRow tconst(m_chopperConfigurationWorkspace->getRow(2));
        tconst << "tconst" << "mysec" << -0.6;

        m_chopperSlitWorkspace = TableWorkspace_sptr(new TableWorkspace(8));
        m_chopperSlitWorkspace->addColumn(std::string("int"), std::string("slits"));
        m_chopperSlitWorkspace->addColumn(std::string("double"), std::string("position"));
        
        ColumnVector<int> slits(m_chopperSlitWorkspace->getVector(std::string("slits")));
        for(size_t i = 0; i < slits.size(); ++i) {
            slits[i] = static_cast<int>(i) + 1;
        }

        double rawSlitPositions[] = {0.000000, 0.162156, 0.250867, 0.3704, 0.439811, 0.588455, 0.761389, 0.895667};
        std::vector<double> slitPositions(rawSlitPositions, rawSlitPositions + sizeof(rawSlitPositions) / sizeof(rawSlitPositions[0]));

        ColumnVector<double> slitPositionsWorkspace(m_chopperSlitWorkspace->getVector(std::string("position")));
        for(size_t i = 0; i < slitPositions.size(); ++i) {
            slitPositionsWorkspace[i] = slitPositions[i];
        }

        m_rotationSpeedWorkspace = TableWorkspace_sptr(new TableWorkspace(1));
        m_rotationSpeedWorkspace->addColumn(std::string("str"), std::string("param"));
        m_rotationSpeedWorkspace->addColumn(std::string("str"), std::string("path"));
        m_rotationSpeedWorkspace->addColumn(std::string("str"), std::string("value"));

        TableRow chopperSpeed(m_rotationSpeedWorkspace->getRow(0));
        chopperSpeed << std::string("ChopperSpeed") << std::string("") << std::string("10000");
    }


    void testChopperInterface()
    {
        Mantid::Poldi::PoldiBasicChopper *basicChopper = new Mantid::Poldi::PoldiBasicChopper();
        TS_ASSERT(basicChopper);

        Mantid::Poldi::PoldiAbstractChopper *abstractChopper = static_cast<Mantid::Poldi::PoldiAbstractChopper *>(basicChopper);
        TS_ASSERT(abstractChopper);

        Mantid::Poldi::PoldiBasicChopper *reCastBasicChopper = dynamic_cast<Mantid::Poldi::PoldiBasicChopper *>(abstractChopper);
        TS_ASSERT(reCastBasicChopper);

        delete basicChopper;
    }

    void testConfigurationLoading()
    {
        Mantid::Poldi::PoldiBasicChopper basicChopper;
        TS_ASSERT_THROWS_NOTHING(basicChopper.loadConfiguration(m_chopperConfigurationWorkspace, m_chopperSlitWorkspace, m_rotationSpeedWorkspace));

        for(size_t i = 0; i < m_chopperConfigurationWorkspace->rowCount(); ++i) {
            TableWorkspace_sptr misConfigured(m_chopperConfigurationWorkspace->clone());
            misConfigured->removeRow(i);

            TS_ASSERT_THROWS(basicChopper.loadConfiguration(misConfigured, m_chopperSlitWorkspace, m_rotationSpeedWorkspace), std::runtime_error);
        }

        TableWorkspace_sptr missingSpeed(m_rotationSpeedWorkspace->clone());
        missingSpeed->removeRow(0);
        TS_ASSERT_THROWS(basicChopper.loadConfiguration(m_chopperConfigurationWorkspace, m_chopperSlitWorkspace, missingSpeed), std::runtime_error);
    }

    void testConfigurationCorrectness()
    {
        Mantid::Poldi::PoldiBasicChopper basicChopper;
        basicChopper.loadConfiguration(m_chopperConfigurationWorkspace, m_chopperSlitWorkspace, m_rotationSpeedWorkspace);

        std::vector<double> slitPositions = basicChopper.slitPositions();
        TS_ASSERT_EQUALS(slitPositions.size(), 8);
        TS_ASSERT_DELTA(slitPositions[0], 0.0, 1e-7);
        TS_ASSERT_DELTA(slitPositions[1], 0.162156, 1e-7);

        TS_ASSERT_DELTA(basicChopper.cycleTime(), 1500.0, 1e-7);
        TS_ASSERT_DELTA(basicChopper.distanceFromSample(), 11800.0, 1e-7);
        TS_ASSERT_DELTA(basicChopper.zeroOffset(), 0.15, 1e-7);

        std::vector<double> slitTimes = basicChopper.slitTimes();
        TS_ASSERT_EQUALS(slitTimes.size(), 8);
        TS_ASSERT_DELTA(slitTimes[0], 0.0, 1e-7);
        TS_ASSERT_DELTA(slitTimes[1], 243.234, 1e-3)
    }


};


#endif /* MANTID_SINQ_POLDIBASICCHOPPERTEST_H_ */
