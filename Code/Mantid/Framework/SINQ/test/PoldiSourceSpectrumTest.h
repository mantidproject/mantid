#ifndef POLDISOURCESPECTRUMTEST_H
#define POLDISOURCESPECTRUMTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"
#include "MantidKernel/Interpolation.h"
#include <stdexcept>

using namespace Mantid::Poldi;
using namespace Mantid::Kernel;

class PoldiSourceSpectrumTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiSourceSpectrumTest *createSuite() { return new PoldiSourceSpectrumTest(); }
    static void destroySuite( PoldiSourceSpectrumTest *suite ) { delete suite; }

    void testInterpolationConstructor()
    {
        Interpolation interpolation;

        TS_ASSERT_THROWS_NOTHING(PoldiSourceSpectrum spectrum(interpolation));
    }

    void testInterpolation()
    {
        Interpolation interpolation;
        interpolation.addPoint(0.0, 2.0);
        interpolation.addPoint(1.0, 4.0);
        interpolation.addPoint(2.0, 6.0);

        TS_ASSERT_EQUALS(interpolation.value(0.0), 2.0);
        TS_ASSERT_EQUALS(interpolation.value(2.0), 6.0);

        PoldiSourceSpectrum spectrum(interpolation);
        TS_ASSERT_EQUALS(spectrum.intensity(0.0), 2.0);
        TS_ASSERT_EQUALS(spectrum.intensity(1.0), 4.0);
    }

};

#endif // POLDISOURCESPECTRUMTEST_H
