#ifndef MANTID_SINQ_POLDISPECTRUMLINEARBACKGROUNDTEST_H_
#define MANTID_SINQ_POLDISPECTRUMLINEARBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiSpectrumLinearBackground.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AlgorithmManager.h"

#include "MantidCurveFitting/Jacobian.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;

class PoldiSpectrumLinearBackgroundTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiSpectrumLinearBackgroundTest *createSuite() { return new PoldiSpectrumLinearBackgroundTest(); }
    static void destroySuite( PoldiSpectrumLinearBackgroundTest *suite ) { delete suite; }

    PoldiSpectrumLinearBackgroundTest() :
        m_xValues(20, 1.0)
    {
        FrameworkManager::Instance();
    }


    void testParameterCount()
    {
        PoldiSpectrumLinearBackground function;
        function.initialize();

        TS_ASSERT_EQUALS(function.nParams(), 1);
    }

    void testConstruction()
    {
        IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");

        TS_ASSERT(function);
        TS_ASSERT_EQUALS(function->name(), "PoldiSpectrumLinearBackground");

        boost::shared_ptr<PoldiSpectrumLinearBackground> castedFunction = boost::dynamic_pointer_cast<PoldiSpectrumLinearBackground>(function);

        TS_ASSERT(function);
    }

    void testSetWorkspace()
    {
        IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");
        boost::shared_ptr<PoldiSpectrumLinearBackground> castedFunction = boost::dynamic_pointer_cast<PoldiSpectrumLinearBackground>(function);

        // default is 0
        TS_ASSERT_EQUALS(castedFunction->getTimeBinCount(), 0);

        // invalid workspace, nothing happens
        MatrixWorkspace_const_sptr invalid;
        TS_ASSERT_THROWS_NOTHING(castedFunction->setWorkspace(invalid));
        TS_ASSERT_EQUALS(castedFunction->getTimeBinCount(), 0);

        // valid workspace with 10 bins
        MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace123(1, 10);
        TS_ASSERT_THROWS_NOTHING(castedFunction->setWorkspace(ws));
        TS_ASSERT_EQUALS(castedFunction->getTimeBinCount(), 10);
    }

    void testFunctionValue()
    {
        IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");
        function->setParameter("A1", 2.0);

        FunctionDomain1DSpectrum domainOne(1, m_xValues);
        FunctionValues values(domainOne);
        function->function(domainOne, values);
        for(size_t i = 0; i < values.size(); ++i) {
            TS_ASSERT_EQUALS(values[i], 2.0);
        }

        FunctionDomain1DSpectrum domainTwo(342, m_xValues);
        function->function(domainTwo, values);
        for(size_t i = 0; i < values.size(); ++i) {
            TS_ASSERT_EQUALS(values[i], 684.0);
        }
    }

    void testJacobian()
    {
        IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");
        function->setParameter("A1", 2.0);

        FunctionDomain1DSpectrum domainOne(1, m_xValues);
        Mantid::CurveFitting::Jacobian jacobian(domainOne.size(), function->nParams());
        function->functionDeriv(domainOne, jacobian);

        for(size_t i = 0; i < domainOne.size(); ++i) {
            TS_ASSERT_EQUALS(jacobian.get(i, 0), 1.0);
        }

        FunctionDomain1DSpectrum domainTwo(342, m_xValues);
        function->functionDeriv(domainTwo, jacobian);

        for(size_t i = 0; i < domainTwo.size(); ++i) {
            TS_ASSERT_EQUALS(jacobian.get(i, 0), 342.0);
        }
    }

    void testFit()
    {
        /* Luckily, these are exactly the data described by this function,
         * using A1 = 1.0, so this is used as a test */
        MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(20, 2);

        IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumLinearBackground");
        function->setParameter("A1", 2.0);

        IAlgorithm_sptr fit =  Mantid::API::AlgorithmManager::Instance().create("Fit");
        fit->initialize();

        fit->setProperty("Function", function);
        fit->setProperty("InputWorkspace", ws);
        fit->setProperty("Minimizer", "Levenberg-MarquardtMD");

        fit->execute();

        TS_ASSERT(fit->isExecuted());

        TS_ASSERT_DELTA(function->getParameter(0), 1.0, 1e-13);
        TS_ASSERT_EQUALS(function->getError(0), 0.0);
    }

private:
    std::vector<double> m_xValues;
};


#endif /* MANTID_SINQ_POLDISPECTRUMLINEARBACKGROUNDTEST_H_ */
