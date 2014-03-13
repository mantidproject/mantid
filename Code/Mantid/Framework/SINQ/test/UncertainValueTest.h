#ifndef UNCERTAINVALUETEST_H
#define UNCERTAINVALUETEST_H

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"

using namespace Mantid::Poldi;

class UncertainValueTest : public CxxTest::TestSuite
{
public:
    static UncertainValueTest *createSuite() { return new UncertainValueTest(); }
    static void destroySuite( UncertainValueTest *suite ) { delete suite; }

    void testConstructor()
    {
        UncertainValue value(1.0, 3.0);

        TS_ASSERT_EQUALS(value.value(), 1.0);
        TS_ASSERT_EQUALS(value.error(), 3.0);

        UncertainValue other;

        TS_ASSERT_EQUALS(other.value(), 0.0);
        TS_ASSERT_EQUALS(other.error(), 0.0);
    }

    void testPlainAddition()
    {
        UncertainValue left(1.0, 1.0);
        UncertainValue right(2.0, 2.0);

        UncertainValue sum = UncertainValue::plainAddition(left, right);

        TS_ASSERT_EQUALS(sum.value(), 3.0);
        TS_ASSERT_EQUALS(sum.error(), 3.0);
    }

    void testlessThanError()
    {
        UncertainValue first(1.0, 2.0);
        UncertainValue second(1.0, 3.0);

        TS_ASSERT(UncertainValue::lessThanError(first, second));
    }

    void testvalueToErrorRatio()
    {
        UncertainValue value(2.0, 4.0);

        TS_ASSERT_EQUALS(UncertainValue::valueToErrorRatio(value), 0.5);

        UncertainValue invalid(2.0, 0.0);

        TS_ASSERT_THROWS(UncertainValue::valueToErrorRatio(invalid), std::domain_error);
    }
};

#endif // UNCERTAINVALUETEST_H
