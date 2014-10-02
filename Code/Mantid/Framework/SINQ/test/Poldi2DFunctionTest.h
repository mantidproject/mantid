#ifndef MANTID_SINQ_POLDI2DFUNCTIONTEST_H_
#define MANTID_SINQ_POLDI2DFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/ParamFunction.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;

class Poldi2DFunctionTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static Poldi2DFunctionTest *createSuite() { return new Poldi2DFunctionTest(); }
    static void destroySuite( Poldi2DFunctionTest *suite ) { delete suite; }


    void testTypes()
    {
        boost::shared_ptr<Poldi2DFunction> function2D(new Poldi2DFunction);

        TS_ASSERT(boost::dynamic_pointer_cast<CompositeFunction>(function2D));
        TS_ASSERT(boost::dynamic_pointer_cast<IFunction1DSpectrum>(function2D));
    }

    void testSummation()
    {
        boost::shared_ptr<Poldi2DFunction> function2D(new Poldi2DFunction);

        IFunction_sptr first(new SummingFunction);
        IFunction_sptr second(new SummingFunction);

        function2D->addFunction(first);
        function2D->addFunction(second);

        // x doesn't matter for that function
        std::vector<double> x(10, 1.0);

        FunctionDomain1DSpectrum domain(0, x);
        FunctionValues values(domain);

        // check that functions are added properly
        function2D->function(domain, values);
        TS_ASSERT_EQUALS(values[0], 2.0);
        TS_ASSERT_EQUALS(values[1], 2.0);
        TS_ASSERT_EQUALS(values[9], 2.0);

        // use same values object again, should give the same results
        function2D->function(domain, values);
        TS_ASSERT_EQUALS(values[0], 2.0);
        TS_ASSERT_EQUALS(values[1], 2.0);
        TS_ASSERT_EQUALS(values[9], 2.0);
    }

private:
    /* small test function that behaves like PoldiSpectrumDomainFunction
     * in that it uses FunctionValues::addToCalculated.
     */
    class SummingFunction : public IFunction1DSpectrum, public ParamFunction {
    public:
        std::string name() const { return "SummingFunction"; }

        void function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const
        {
            values.zeroCalculated();

            for(size_t i = 0; i < domain.size(); ++i) {
                values.addToCalculated(i, 1.0);
            }
        }

        void functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) {
            UNUSED_ARG(domain);
            UNUSED_ARG(jacobian);
        }

    };

};


#endif /* MANTID_SINQ_POLDI2DFUNCTIONTEST_H_ */
