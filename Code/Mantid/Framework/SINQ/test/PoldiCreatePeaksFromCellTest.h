#ifndef MANTID_SINQ_POLDICREATEPEAKSFROMCELLTEST_H_
#define MANTID_SINQ_POLDICREATEPEAKSFROMCELLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiCreatePeaksFromCell.h"
#include "MantidAPI/ITableWorkspace.h"

using Mantid::Poldi::PoldiCreatePeaksFromCell;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class PoldiCreatePeaksFromCellTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiCreatePeaksFromCellTest *createSuite() { return new PoldiCreatePeaksFromCellTest(); }
    static void destroySuite( PoldiCreatePeaksFromCellTest *suite ) { delete suite; }


    void test_Init()
    {
        PoldiCreatePeaksFromCell alg;
        TS_ASSERT_THROWS_NOTHING( alg.initialize() )
                TS_ASSERT( alg.isInitialized() )
    }

    void test_exec()
    {
        /* This test checks that the outcome of the algorithm
         * is correct.
         */
        std::string outWSName("PoldiCreatePeaksFromCellTest_OutputWS");

        PoldiCreatePeaksFromCell alg;
        TS_ASSERT_THROWS_NOTHING( alg.initialize() )
        TS_ASSERT( alg.isInitialized() )
        TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("SpaceGroup", "P m -3 m") );
        TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Atoms", "Cl 0 0 0 1.0 0.005; Cs 0.5 0.5 0.5 1.0 0.005");)
        TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("a", "4.126"));
        TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LatticeSpacingMin", "0.55"));
        TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LatticeSpacingMax", "4.0"));
        TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
        TS_ASSERT_THROWS_NOTHING( alg.execute(); );
        TS_ASSERT( alg.isExecuted() );

        // Retrieve the workspace from data service.
        Workspace_sptr ws;
        TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
        TS_ASSERT(ws);

        ITableWorkspace_sptr tableWs = boost::dynamic_pointer_cast<ITableWorkspace>(ws);

        TS_ASSERT(tableWs);
        // There should be 68 unique reflections for this cell and d-range.
        TS_ASSERT_EQUALS(tableWs->rowCount(), 68);

        if (ws) {
            AnalysisDataService::Instance().remove(outWSName);
        }
    }

    void testValidateInput()
    {
        PoldiCreatePeaksFromCell alg;
        alg.initialize();

        alg.setPropertyValue("LatticeSpacingMin", "1.0");
        alg.setPropertyValue("LatticeSpacingMax", "2.0");

        // dMax is larger than dMin
        std::map<std::string, std::string> errorMap = alg.validateInputs();
        TS_ASSERT_EQUALS(errorMap.size(), 0);

        alg.setPropertyValue("LatticeSpacingMax", "0.5");
        // now it's smaller - not allowed
        errorMap = alg.validateInputs();
        TS_ASSERT_EQUALS(errorMap.size(), 1);

        errorMap.clear();

        alg.setPropertyValue("LatticeSpacingMax", "-0.5");
        errorMap = alg.validateInputs();
        TS_ASSERT_EQUALS(errorMap.size(), 1)
    }

    void testGetLargestDValue()
    {
        // Maximum d-value is 30.0
        UnitCell cell(10.0, 20.0, 30.0);
        TestablePoldiCreatePeaksFromCell alg;

        TS_ASSERT_EQUALS(alg.getLargestDValue(cell), 30.0);
    }

    void testGetDMaxValue()
    {
        // Maximum d-value is 30.0
        UnitCell cell(10.0, 20.0, 30.0);

        TestablePoldiCreatePeaksFromCell alg;
        alg.initialize();

        // dMax has default value - largest d-value + 1.0 is supposed to be returned
        TS_ASSERT_EQUALS(alg.getDMaxValue(cell), 31.0);

        // dMax has been set to a different value
        alg.setPropertyValue("LatticeSpacingMax", "2.0");
        TS_ASSERT_EQUALS(alg.getDMaxValue(cell), 2.0);

        alg.setPropertyValue("LatticeSpacingMax", "100.0");
        TS_ASSERT_EQUALS(alg.getDMaxValue(cell), 100.0);
    }

    void testGetUnitCellFromProperties()
    {
        TestablePoldiCreatePeaksFromCell alg;
        alg.initialize();

        alg.setPropertyValue("a", "3.0");
        alg.setPropertyValue("b", "4.0");
        alg.setPropertyValue("c", "5.0");

        alg.setPropertyValue("alpha", "90.0");
        alg.setPropertyValue("beta", "91.0");
        alg.setPropertyValue("gamma", "92.0");

        UnitCell unitCell = alg.getUnitCellFromProperties();

        TS_ASSERT_EQUALS(unitCell.a(), 3.0);
        TS_ASSERT_EQUALS(unitCell.b(), 4.0);
        TS_ASSERT_EQUALS(unitCell.c(), 5.0);
        TS_ASSERT_EQUALS(unitCell.alpha(), 90.0);
        TS_ASSERT_EQUALS(unitCell.beta(), 91.0);
        TS_ASSERT_EQUALS(unitCell.gamma(), 92.0);
    }

    void testGetConstrainedUnitCell()
    {
        TestablePoldiCreatePeaksFromCell alg;

        UnitCell rawCell(2.0, 3.0, 4.0, 91.0, 92.0, 93.0);

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Cubic),
                    2.0, 2.0, 2.0, 90.0, 90.0, 90.0, "Cubic"
                    );

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Tetragonal),
                    2.0, 2.0, 4.0, 90.0, 90.0, 90.0, "Tetragonal"
                    );

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Orthorhombic),
                    2.0, 3.0, 4.0, 90.0, 90.0, 90.0, "Orthorhombic"
                    );

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Monoclinic),
                    2.0, 3.0, 4.0, 90.0, 92.0, 90.0, "Monoclinic"
                    );

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Triclinic),
                    2.0, 3.0, 4.0, 91.0, 92.0, 93.0, "Triclinic"
                    );

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Hexagonal),
                    2.0, 2.0, 4.0, 90.0, 90.0, 120.0, "Hexagonal"
                    );

        checkUnitCellParameters(
                    alg.getConstrainedUnitCell(rawCell, PointGroup::Trigonal),
                    2.0, 2.0, 2.0, 91.0, 91.0, 91.0, "Trigonal"
                    );
    }

private:
    void checkUnitCellParameters(const UnitCell &cell, double a, double b, double c, double alpha, double beta, double gamma, const std::string &message)
    {
        TSM_ASSERT_DELTA(message, cell.a(), a, 1e-14);
        TSM_ASSERT_DELTA(message, cell.b(), b, 1e-14);
        TSM_ASSERT_DELTA(message, cell.c(), c, 1e-14);

        TSM_ASSERT_DELTA(message, cell.alpha(), alpha, 1e-14);
        TSM_ASSERT_DELTA(message, cell.beta(), beta, 1e-14);
        TSM_ASSERT_DELTA(message, cell.gamma(), gamma, 1e-14);
    }

    class TestablePoldiCreatePeaksFromCell : public PoldiCreatePeaksFromCell
    {
    public:
        TestablePoldiCreatePeaksFromCell() : PoldiCreatePeaksFromCell()
        { }
        ~TestablePoldiCreatePeaksFromCell() { }

        friend class PoldiCreatePeaksFromCellTest;
    };


};


#endif /* MANTID_SINQ_POLDICREATEPEAKSFROMCELLTEST_H_ */
