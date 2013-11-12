#ifndef BSPLINETEST_H_
#define BSPLINETEST_H_

#include "MantidCurveFitting/BSpline.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"

#include <cxxtest/TestSuite.h>
#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class BSplineTest: public CxxTest::TestSuite
{
public:

    void test_defaults()
    {
        BSpline bsp;
        int order = bsp.getAttribute("Order").asInt();
        int nbreak = bsp.getAttribute("NBreak").asInt();
        size_t nparams = bsp.nParams();

        TS_ASSERT_EQUALS( order, 3 );
        TS_ASSERT_EQUALS( nbreak, 10 );
        TS_ASSERT_EQUALS( nparams, 11 );
        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), 0.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), 1.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("Uniform").asBool(), true );

    }

    void test_set_uniform_break_points()
    {
        BSpline bsp;
        TS_ASSERT_EQUALS( bsp.getAttribute("Uniform").asBool(), true );
        TS_ASSERT_EQUALS( bsp.getAttribute("NBreak").asInt(), 10 );
        bsp.setAttributeValue("StartX",-10.0);
        bsp.setAttributeValue("EndX", 10.0);
        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), -10.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), 10.0 );

        std::vector<double> breaks = bsp.getAttribute("BreakPoints").asVector();
        TS_ASSERT_EQUALS( breaks.size(), 10 );
        TS_ASSERT_EQUALS( bsp.nParams(), 11 );

        const double dx = 20.0 / 9;
        for(size_t i = 0; i < 10; ++i)
        {
            TS_ASSERT_DELTA( -10.0 + static_cast<double>(i)*dx, breaks[i], 1e-14 );
            TS_ASSERT_EQUALS( bsp.parameterName(i), "A" + boost::lexical_cast<std::string>(i) );
        }
        TS_ASSERT_EQUALS( bsp.parameterName(10), "A10" );
    }

    void test_set_nonuniform_break_points()
    {
        BSpline bsp;
        bsp.setAttributeValue("Uniform",false);
        std::vector<double> inputBreaks(8);
        inputBreaks[0] = 3.0;
        inputBreaks[1] = 4.0;
        inputBreaks[2] = 7.0;
        inputBreaks[3] = 8.0;
        inputBreaks[4] = 15.0;
        inputBreaks[5] = 17.0;
        inputBreaks[6] = 18.0;
        inputBreaks[7] = 30.0;
        bsp.setAttributeValue("BreakPoints", inputBreaks);

        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), 3.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), 30.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("NBreak").asInt(), 8 );

        std::vector<double> breaks = bsp.getAttribute("BreakPoints").asVector();
        TS_ASSERT_EQUALS( breaks.size(), 8 );
        TS_ASSERT_EQUALS( bsp.nParams(), 9 );
        for(size_t i = 0; i < 8; ++i)
        {
            TS_ASSERT_DELTA( inputBreaks[i], breaks[i], 1e-14 );
            TS_ASSERT_EQUALS( bsp.parameterName(i), "A" + boost::lexical_cast<std::string>(i) );
        }
        TS_ASSERT_EQUALS( bsp.parameterName(8), "A8" );
    }

    void test_try_set_nonuniform_break_points_with_wrong_order()
    {
        BSpline bsp;
        bsp.setAttributeValue("Uniform",false);
        std::vector<double> inputBreaks(8);
        inputBreaks[0] = 3.0;
        inputBreaks[1] = 4.0;
        inputBreaks[2] = 7.0;
        inputBreaks[3] = 8.0;
        inputBreaks[4] = 15.0;
        inputBreaks[5] = 7.0;
        inputBreaks[6] = 18.0;
        inputBreaks[7] = 30.0;
        TS_ASSERT_THROWS( bsp.setAttributeValue("BreakPoints", inputBreaks), std::invalid_argument );
    }

    void test_set_wrong_startx_endx()
    {
        BSpline bsp;
        TS_ASSERT_EQUALS( bsp.getAttribute("Uniform").asBool(), true );
        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), 0.0 );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), 1.0 );

        double startx = 10.0;
        double endx = -10.0;

        bsp.setAttributeValue("StartX",startx);
        bsp.setAttributeValue("EndX", endx);

        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), startx );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), endx );

        FunctionDomain1DVector  x(startx,endx,100);
        FunctionValues y(x);

        TS_ASSERT_THROWS( bsp.function(x,y), std::invalid_argument );

        startx = 10.0;
        endx = startx;

        bsp.setAttributeValue("StartX",startx);
        bsp.setAttributeValue("EndX", endx);

        TS_ASSERT_EQUALS( bsp.getAttribute("StartX").asDouble(), startx );
        TS_ASSERT_EQUALS( bsp.getAttribute("EndX").asDouble(), endx );

        FunctionDomain1DVector  x1(startx,endx,100);
        FunctionValues y1(x1);

        TS_ASSERT_THROWS( bsp.function(x1,y1), std::invalid_argument );

    }

    void test_fit_uniform()
    {
        double startx = -3.14;
        double endx = 3.14;

        boost::shared_ptr<BSpline> bsp(new BSpline);
        bsp->setAttributeValue("Order",3);
        bsp->setAttributeValue("NBreak",10);
        bsp->setAttributeValue("StartX",startx);
        bsp->setAttributeValue("EndX", endx);

        double chi2 = fit(bsp,"sin(x)");
        TS_ASSERT_DELTA( chi2, 1e-4, 1e-5 );

        FunctionDomain1DVector  x(startx,endx,100);
        FunctionValues y(x);
        bsp->function(x,y);

        for(size_t i = 0; i < x.size(); ++i)
        {
            double xx = x[i];
            TS_ASSERT_DELTA( y[i], sin(xx), 0.003 );
        }
    }

    void test_fit_uniform_finer()
    {
        double startx = -3.14;
        double endx = 3.14;

        boost::shared_ptr<BSpline> bsp(new BSpline);
        bsp->setAttributeValue("Order",3);
        bsp->setAttributeValue("NBreak",20);
        bsp->setAttributeValue("StartX",startx);
        bsp->setAttributeValue("EndX", endx);

        double chi2 = fit(bsp,"sin(x)");
        TS_ASSERT_DELTA( chi2, 1e-6, 1e-7 );

        FunctionDomain1DVector  x(startx,endx,100);
        FunctionValues y(x);
        bsp->function(x,y);

        for(size_t i = 0; i < x.size(); ++i)
        {
            double xx = x[i];
            TS_ASSERT_DELTA( y[i], sin(xx), 0.0003 );
        }
    }

    void test_fit_nonuniform()
    {
        double startx = 0.0;
        double endx = 6.28;

        boost::shared_ptr<BSpline> bsp(new BSpline);
        bsp->setAttributeValue("Order",3);
        bsp->setAttributeValue("NBreak",10);
        bsp->setAttributeValue("StartX",startx);
        bsp->setAttributeValue("EndX", endx);

        // this function changes faster at the lower end
        // fit it with uniform break points first
        double chi2 = fit(bsp,"sin(10/(x+1))");
        TS_ASSERT_DELTA( chi2, 0.58, 0.005 );

        // now do a nonuniform fit. increase density of break points at lower end
        std::vector<double> breaks = bsp->getAttribute("BreakPoints").asVector();
        breaks[1] = 0.3;
        breaks[2] = 0.5;
        breaks[3] = 1.0;
        breaks[4] = 1.5;
        breaks[5] = 2.0;
        breaks[6] = 3.0;
        bsp->setAttributeValue("Uniform",false);
        bsp->setAttributeValue("BreakPoints",breaks);
        chi2 = fit(bsp,"sin(10/(x+1))");
        TS_ASSERT_DELTA( chi2, 0.0055, 5e-5 );

    }

    void test_create_with_function_factory_uniform()
    {
        auto bsp = FunctionFactory::Instance().createInitialized("name=BSpline,Uniform=true,Order=3,NBreak=3,StartX=0.05,EndX=66.6,BreakPoints=(0.005,0.5,6.0)");
        TS_ASSERT_EQUALS( bsp->getAttribute("StartX").asDouble(), 0.05 );
        TS_ASSERT_EQUALS( bsp->getAttribute("EndX").asDouble(), 66.6 );
        TS_ASSERT_EQUALS( bsp->getAttribute("Uniform").asBool(), true );
        TS_ASSERT_EQUALS( bsp->getAttribute("NBreak").asInt(), 3 );
        std::vector<double> breaks = bsp->getAttribute("BreakPoints").asVector();
        TS_ASSERT_EQUALS( breaks.size(), 3 );
        TS_ASSERT_EQUALS( breaks[0], 0.05 );
        TS_ASSERT_DELTA( breaks[1], 33.325, 1e-14 );
        TS_ASSERT_EQUALS( breaks[2], 66.6 );
    }

    void test_create_with_function_factory_nonuniform()
    {
        auto bsp = FunctionFactory::Instance().createInitialized("name=BSpline,Uniform=false,Order=3,NBreak=3,StartX=0.05,EndX=66.6,BreakPoints=(0.005,0.5,6.0)");
        TS_ASSERT_EQUALS( bsp->getAttribute("StartX").asDouble(), 0.005 );
        TS_ASSERT_EQUALS( bsp->getAttribute("EndX").asDouble(), 6.0 );
        TS_ASSERT_EQUALS( bsp->getAttribute("Uniform").asBool(), false );
        TS_ASSERT_EQUALS( bsp->getAttribute("NBreak").asInt(), 3 );
        std::vector<double> breaks = bsp->getAttribute("BreakPoints").asVector();
        TS_ASSERT_EQUALS( breaks.size(), 3 );
        TS_ASSERT_EQUALS( breaks[0], 0.005 );
        TS_ASSERT_EQUALS( breaks[1], 0.5 );
        TS_ASSERT_EQUALS( breaks[2], 6.0 );
    }

    void test_derivative()
    {

        double startx = -3.14;
        double endx = 3.14;

        boost::shared_ptr<BSpline> bsp(new BSpline);
        bsp->setAttributeValue("Order",3);
        bsp->setAttributeValue("NBreak",30);
        bsp->setAttributeValue("StartX",startx);
        bsp->setAttributeValue("EndX", endx);

        double chi2 = fit(bsp,"sin(x)");
        TS_ASSERT_DELTA( chi2, 1e-7, 5e-8 );

        FunctionDomain1DVector  x(startx,endx,100);
        FunctionValues y(x);
        bsp->derivative(x,y); // first derivative

        for(size_t i = 0; i < x.size(); ++i)
        {
            double xx = x[i];
            TS_ASSERT_DELTA( y[i], cos(xx), 0.005 );
        }
    }

    void test_derivative_2()
    {

        double startx = -3.14;
        double endx = 3.14;

        boost::shared_ptr<BSpline> bsp(new BSpline);
        bsp->setAttributeValue("Order",4);
        bsp->setAttributeValue("NBreak",30);
        bsp->setAttributeValue("StartX",startx);
        bsp->setAttributeValue("EndX", endx);

        double chi2 = fit(bsp,"sin(x)");
        TS_ASSERT_DELTA( chi2, 2e-10, 1e-10 );

        FunctionDomain1DVector  x(startx,endx,100);
        FunctionValues y(x);
        bsp->derivative(x,y,2); // second derivative

        for(size_t i = 0; i < x.size(); ++i)
        {
            double xx = x[i];
            TS_ASSERT_DELTA( y[i], -sin(xx), 0.005 );
        }
    }

    void test_derivative_3()
    {

        double startx = -3.14;
        double endx = 3.14;

        boost::shared_ptr<BSpline> bsp(new BSpline);
        bsp->setAttributeValue("Order",5);
        bsp->setAttributeValue("NBreak",20);
        bsp->setAttributeValue("StartX",startx);
        bsp->setAttributeValue("EndX", endx);

        double chi2 = fit(bsp,"sin(x)");
        TS_ASSERT_DELTA( chi2, 1e-11, 5e-12 );

        FunctionDomain1DVector  x(startx,endx,100);
        FunctionValues y(x);
        bsp->derivative(x,y,3); // third derivative

        for(size_t i = 0; i < x.size(); ++i)
        {
            double xx = x[i];
            TS_ASSERT_DELTA( y[i], -cos(xx), 0.012 );
        }
    }

    void test_nonpositive_order()
    {
        BSpline bsp;
        TS_ASSERT_THROWS( bsp.setAttributeValue("Order",-3), std::invalid_argument );
        TS_ASSERT_THROWS( bsp.setAttributeValue("Order",0), std::invalid_argument );
    }

    void test_nbreak_too_small()
    {
        BSpline bsp;
        TS_ASSERT_THROWS( bsp.setAttributeValue("NBreak",1), std::invalid_argument );
        TS_ASSERT_THROWS( bsp.setAttributeValue("NBreak",0), std::invalid_argument );
        TS_ASSERT_THROWS( bsp.setAttributeValue("NBreak",-3), std::invalid_argument );
    }

private:

    double fit(boost::shared_ptr<IFunction> bsp, std::string func)
    {
        const double startx = bsp->getAttribute("StartX").asDouble();
        const double endx = bsp->getAttribute("EndX").asDouble();

        API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(startx,endx,100));
        API::FunctionValues mockData(*domain);
        UserFunction dataMaker;
        dataMaker.setAttributeValue("Formula",func);
        dataMaker.function(*domain,mockData);

        API::FunctionValues_sptr values(new API::FunctionValues(*domain));
        values->setFitDataFromCalculated(mockData);
        values->setFitWeights(1.0);

        boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
        costFun->setFittingFunction(bsp,domain,values);

        LevenbergMarquardtMDMinimizer s;
        s.initialize(costFun);
        TS_ASSERT(s.minimize());
        return costFun->val();
    }

};

#endif /*BSPLINETEST_H_*/
