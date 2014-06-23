#ifndef INTERPOLATIONTEST_H_
#define INTERPOLATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <ctime>
#include "MantidKernel/Interpolation.h"

using namespace Mantid::Kernel;

class InterpolationTest : public CxxTest::TestSuite
{
public:
    /* In the constructor some vectors with values are setup,
     * which make the tests easier later on.
     *
     * To check the interpolated values, call the method
     *   checkInterpolationResults(const Interpolation &interpolation);
     * and supply the interpolation object which is to be tested. The method will call
     * further methods to cover all possible edge-cases. On failure, it will be visible
     * which case caused the failure.
     */
    InterpolationTest()
    {
        // values for setting up the interpolation
        m_tableXValues.push_back(200.0);
        m_tableXValues.push_back(201.0);
        m_tableXValues.push_back(202.0);
        m_tableXValues.push_back(203.0);
        m_tableXValues.push_back(204.0);

        m_tableYValues.push_back(50);
        m_tableYValues.push_back(60);
        m_tableYValues.push_back(100);
        m_tableYValues.push_back(300);
        m_tableYValues.push_back(400);

        // bulk values for interpolation test
        m_interpolationXValues.push_back(200.5);
        m_interpolationXValues.push_back(201.25);
        m_interpolationXValues.push_back(203.5);

        m_expectedYValues.push_back(55.0);
        m_expectedYValues.push_back(70.0);
        m_expectedYValues.push_back(350.0);

        // values outside interpolation range
        m_outsideXValues.push_back(100.0);
        m_outsideXValues.push_back(3000.0);

        m_outsideYValues.push_back(-950.0);
        m_outsideYValues.push_back(280000.0);
    }

    void testCopyConstruction()
    {
        Interpolation interpolation;
        interpolation.setMethod("linear");
        interpolation.setXUnit("Wavelength");
        interpolation.setYUnit("dSpacing");

        interpolation.addPoint(200.0, 2.0);

        Interpolation other = interpolation;

        TS_ASSERT_EQUALS(other.getMethod(), "linear");
        TS_ASSERT_EQUALS(other.getXUnit()->unitID(), "Wavelength");
        TS_ASSERT_EQUALS(other.getYUnit()->unitID(), "dSpacing");
        TS_ASSERT_EQUALS(other.value(200.0), 2.0);
    }

    void testContainData()
    {
        Interpolation interpolation;

        TS_ASSERT ( interpolation.containData() == false );

        interpolation.addPoint(200.0, 50);

        TS_ASSERT ( interpolation.containData() == true );
    }

    void testResetData()
    {
        Interpolation interpolation = getInitializedInterpolation("Wavelength", "dSpacing");

        TS_ASSERT(interpolation.containData());
        interpolation.resetData();
        TS_ASSERT(interpolation.containData() == false);
    }

    void testAddPointOrdered()
    {
        Interpolation interpolation;

        // Add points from values in vectors in correct order.
        for(size_t i = 0; i < m_tableXValues.size(); ++i) {
            interpolation.addPoint(m_tableXValues[i], m_tableYValues[i]);
        }

        // Check correctness of interpolation for different cases
        checkInterpolationResults(interpolation);
    }

    void testAddPointArbitrary()
    {
        Interpolation interpolation;

        std::vector<size_t> insertionOrder = {1, 0, 3, 4, 2};
        for(std::vector<size_t>::const_iterator i = insertionOrder.begin(); i != insertionOrder.end(); ++i) {
            interpolation.addPoint(m_tableXValues[*i], m_tableYValues[*i]);
        }

        checkInterpolationResults(interpolation);
    }

    void testEmpty()
    {
        Interpolation interpolation;

        std::stringstream str;
        str << interpolation;
        TS_ASSERT( str.str().compare("linear ; TOF ; TOF") == 0 );

        Interpolation readIn;
        str >> readIn;

        TS_ASSERT( readIn.containData() == false );
    }

    void testStreamOperators()
    {
        std::string xUnit = "Wavelength";
        std::string yUnit = "dSpacing";

        Interpolation interpolation = getInitializedInterpolation(xUnit, yUnit);

        // Output stream
        std::stringstream str;
        str << interpolation;
        TS_ASSERT( str.str().compare("linear ; Wavelength ; dSpacing ; 200 50 ; 201 60 ; 202 100 ; 203 300 ; 204 400") == 0 );

        // Input stream for empty interpolation object
        Interpolation readIn;
        TS_ASSERT( readIn.getXUnit()->unitID() == "TOF" );
        TS_ASSERT( readIn.getYUnit()->unitID() == "TOF" );
        str >> readIn;
        TS_ASSERT( readIn.getXUnit()->unitID() == xUnit );
        TS_ASSERT( readIn.getYUnit()->unitID() == yUnit );

        checkInterpolationResults(readIn);
    }

    void testStreamOperatorsNonEmpty()
    {
        Interpolation interpolation = getInitializedInterpolation("Wavelength", "dSpacing");

        std::stringstream str;
        str << interpolation;

        // Reconstruct on existing object.
        str >> interpolation;

        checkInterpolationResults(interpolation);
    }

private:
    Interpolation getInitializedInterpolation(std::string xUnit, std::string yUnit)
    {
        Interpolation interpolation;

        // take values from constructor
        for(size_t i = 0; i < m_tableXValues.size(); ++i) {
            interpolation.addPoint(m_tableXValues[i], m_tableYValues[i]);
        }

        interpolation.setXUnit(xUnit);
        interpolation.setYUnit(yUnit);

        return interpolation;
    }

    void checkInterpolationResults(const Interpolation &interpolation)
    {
        checkValueAtLowerLimit(interpolation);
        checkValueAtUpperLimit(interpolation);
        checkValuesAtExactBulkPoints(interpolation);
        checkValuesInsideInterpolationRange(interpolation);
        checkValuesOutsideInterpolationRange(interpolation);
    }

    void checkValueAtLowerLimit(const Interpolation &interpolation)
    {
        checkValue(interpolation, m_tableXValues.front(), m_tableYValues.front(), "at lower limit");
    }

    void checkValueAtUpperLimit(const Interpolation &interpolation)
    {
        checkValue(interpolation, m_tableXValues.back(), m_tableYValues.back(), "at upper limit");
    }

    void checkValuesAtExactBulkPoints(const Interpolation &interpolation)
    {
        for(size_t i = 1; i < m_tableXValues.size() - 1; ++i) {
            checkValue(interpolation, m_tableXValues[i], m_tableYValues[i], "at interpolation point");
        }
    }

    void checkValuesInsideInterpolationRange(const Interpolation &interpolation) {
        for(size_t i = 0; i < m_interpolationXValues.size(); ++i) {
            checkValue(interpolation, m_interpolationXValues[i], m_expectedYValues[i], "inside interpolation range");
        }
    }

    void checkValuesOutsideInterpolationRange(const Interpolation &interpolation)
    {
        for(size_t i = 0; i < m_outsideXValues.size(); ++i) {
            checkValue(interpolation, m_outsideXValues[i], m_outsideYValues[i], "outside interpolation range");
        }
    }

    /* This function performs the actual check.
     * It takes a string argument to make it more obvious where the problem is.
     */
    void checkValue(const Interpolation &interpolation, double x, double y, std::string testedRange)
    {
        std::ostringstream errorString;
        errorString << "Interpolation error " << testedRange;

        TSM_ASSERT_EQUALS(errorString.str().c_str(), interpolation.value(x), y);
    }

    // These two vectors contain the data points from which the interpolation is constructed
    std::vector<double> m_tableXValues;
    std::vector<double> m_tableYValues;
  
    // Two vectors with test values for the "bulk", e.g. no values at the limits and
    std::vector<double> m_interpolationXValues;
    std::vector<double> m_expectedYValues;

    // Values outside interpolation range
    std::vector<double> m_outsideXValues;
    std::vector<double> m_outsideYValues;
};

#endif /*INTERPOLATIONTEST_H_*/
