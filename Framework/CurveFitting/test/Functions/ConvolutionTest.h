#ifndef CONVOLUTIONTEST_H_
#define CONVOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/DeltaFunction.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"

#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class ConvolutionExpression {
public:
  double operator()(double x) {
    return 1 + 0.3 * x + exp(-0.5 * (x - 4) * (x - 4) * 2) +
           2 * exp(-0.5 * (x - 6) * (x - 6) * 3);
  }
};

class ConvolutionExp {
public:
  double operator()(double x) { return exp(-0.5 * (x - 7) * (x - 7) * 2); }
};

class ConvolutionTest_Gauss : public IPeakFunction {
public:
  ConvolutionTest_Gauss() {
    declareParameter("c");
    declareParameter("h", 1.);
    declareParameter("s", 1.);
  }

  std::string name() const override { return "ConvolutionTest_Gauss"; }

  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      out[i] = h * exp(-x * x * w);
    }
  }
  void functionDerivLocal(Jacobian *out, const double *xValues,
                          const size_t nData) override {
    // throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      double e = h * exp(-x * x * w);
      out->set(i, 0, x * h * e * w);
      out->set(i, 1, e);
      out->set(i, 2, -x * x * h * e);
    }
  }

  double centre() const override { return getParameter(0); }

  double height() const override { return getParameter(1); }

  double fwhm() const override { return getParameter(2); }

  void setCentre(const double c) override { setParameter(0, c); }
  void setHeight(const double h) override { setParameter(1, h); }

  void setFwhm(const double w) override { setParameter(2, w); }
};

class ConvolutionTest_Lorentz : public IPeakFunction {
public:
  ConvolutionTest_Lorentz() {
    declareParameter("c");
    declareParameter("h", 1.);
    declareParameter("w", 1.);
  }

  std::string name() const override { return "ConvolutionTest_Lorentz"; }

  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override {
    const double height = getParameter("h");
    const double peakCentre = getParameter("c");
    const double hwhm = getParameter("w");

    for (size_t i = 0; i < nData; i++) {
      double diff = xValues[i] - peakCentre;
      out[i] = height * (hwhm * hwhm / (diff * diff + hwhm * hwhm));
    }
  }

  void functionDerivLocal(Jacobian *out, const double *xValues,
                          const size_t nData) override {
    const double height = getParameter("h");
    const double peakCentre = getParameter("c");
    const double hwhm = getParameter("w");

    for (size_t i = 0; i < nData; i++) {
      double diff = xValues[i] - peakCentre;
      double invDenominator = 1 / ((diff * diff + hwhm * hwhm));
      out->set(i, 0, hwhm * hwhm * invDenominator);
      out->set(i, 1, 2.0 * height * diff * hwhm * hwhm * invDenominator *
                         invDenominator);
      out->set(i, 2, height * (-hwhm * hwhm * invDenominator + 1) * 2.0 * hwhm *
                         invDenominator);
    }
  }

  double centre() const override { return getParameter(0); }
  double height() const override { return getParameter(1); }
  double fwhm() const override { return getParameter(2); }

  void setCentre(const double c) override { setParameter(0, c); }
  void setHeight(const double h) override { setParameter(1, h); }
  void setFwhm(const double w) override { setParameter(2, w); }
};

class ConvolutionTest_Linear : public ParamFunction, public IFunction1D {
public:
  ConvolutionTest_Linear() {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name() const override { return "ConvolutionTest_Linear"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override {
    double a = getParameter("a");
    double b = getParameter("b");
    for (size_t i = 0; i < nData; i++) {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) override {
    // throw Mantid::Kernel::Exception::NotImplementedError("");
    for (size_t i = 0; i < nData; i++) {
      out->set(i, 0, 1.);
      out->set(i, 1, xValues[i]);
    }
  }
};

DECLARE_FUNCTION(ConvolutionTest_Gauss)
DECLARE_FUNCTION(ConvolutionTest_Lorentz)
DECLARE_FUNCTION(ConvolutionTest_Linear)

class ConvolutionTest : public CxxTest::TestSuite {
public:
  void testFunction() {
    Convolution conv;

    IFunction_sptr gauss1(new ConvolutionTest_Gauss);
    gauss1->setParameter(0, 1.1);
    gauss1->setParameter(1, 1.2);
    gauss1->setParameter(2, 1.3);
    IFunction_sptr gauss2(new ConvolutionTest_Gauss);
    gauss2->setParameter(0, 2.1);
    gauss2->setParameter(1, 2.2);
    gauss2->setParameter(2, 2.3);
    IFunction_sptr gauss3(new ConvolutionTest_Gauss);
    gauss3->setParameter(0, 3.1);
    gauss3->setParameter(1, 3.2);
    gauss3->setParameter(2, 3.3);
    IFunction_sptr linear(new ConvolutionTest_Linear);
    linear->setParameter(0, 0.1);
    linear->setParameter(1, 0.2);

    size_t iFun = 10000;
    iFun = conv.addFunction(linear);
    TS_ASSERT_EQUALS(iFun, 0);
    iFun = conv.addFunction(gauss1);
    TS_ASSERT_EQUALS(iFun, 1);
    iFun = conv.addFunction(gauss2);
    TS_ASSERT_EQUALS(iFun, 1);
    iFun = conv.addFunction(gauss3);
    TS_ASSERT_EQUALS(iFun, 1);

    TS_ASSERT_EQUALS(conv.nFunctions(), 2);
    TS_ASSERT_EQUALS(conv.name(), "Convolution");

    CompositeFunction *cf =
        dynamic_cast<CompositeFunction *>(conv.getFunction(1).get());
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(conv.nParams(), 11);
    TS_ASSERT_EQUALS(conv.parameterName(0), "f0.a");
    TS_ASSERT_EQUALS(conv.getParameter(0), 0.1);
    TS_ASSERT_EQUALS(conv.parameterName(2), "f1.f0.c");
    TS_ASSERT_EQUALS(conv.getParameter(2), 1.1);
    TS_ASSERT_EQUALS(conv.parameterName(6), "f1.f1.h");
    TS_ASSERT_EQUALS(conv.getParameter(6), 2.2);
    TS_ASSERT_EQUALS(conv.parameterName(10), "f1.f2.s");
    TS_ASSERT_EQUALS(conv.getParameter(10), 3.3);

    TS_ASSERT_EQUALS(conv.nameOfActive(2), "f1.f0.c");
    TS_ASSERT_EQUALS(conv.activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(conv.nameOfActive(6), "f1.f1.h");
    TS_ASSERT_EQUALS(conv.activeParameter(6), 2.2);
    TS_ASSERT_EQUALS(conv.nameOfActive(10), "f1.f2.s");
    TS_ASSERT_EQUALS(conv.activeParameter(10), 3.3);

    TS_ASSERT_EQUALS(conv.parameterLocalName(0), "a");
    TS_ASSERT_EQUALS(conv.parameterLocalName(2), "f0.c");
    TS_ASSERT_EQUALS(conv.parameterLocalName(6), "f1.h");
    TS_ASSERT_EQUALS(conv.parameterLocalName(10), "f2.s");

    IFunction_sptr fun =
        FunctionFactory::Instance().createInitialized(conv.asString());
    TS_ASSERT(fun);

    Convolution *conv1 = dynamic_cast<Convolution *>(fun.get());
    TS_ASSERT(conv1);

    TS_ASSERT_EQUALS(conv1->nFunctions(), 2);
    TS_ASSERT_EQUALS(conv1->name(), "Convolution");

    CompositeFunction *cf1 =
        dynamic_cast<CompositeFunction *>(conv1->getFunction(1).get());
    TS_ASSERT(cf1);
    TS_ASSERT_EQUALS(conv1->nParams(), 11);
    TS_ASSERT_EQUALS(conv1->parameterName(0), "f0.a");
    TS_ASSERT_EQUALS(conv1->getParameter(0), 0.1);
    TS_ASSERT_EQUALS(conv1->parameterName(2), "f1.f0.c");
    TS_ASSERT_EQUALS(conv1->getParameter(2), 1.1);
    TS_ASSERT_EQUALS(conv1->parameterName(6), "f1.f1.h");
    TS_ASSERT_EQUALS(conv1->getParameter(6), 2.2);
    TS_ASSERT_EQUALS(conv1->parameterName(10), "f1.f2.s");
    TS_ASSERT_EQUALS(conv1->getParameter(10), 3.3);

    TS_ASSERT_EQUALS(conv1->nameOfActive(2), "f1.f0.c");
    TS_ASSERT_EQUALS(conv1->activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(conv1->nameOfActive(6), "f1.f1.h");
    TS_ASSERT_EQUALS(conv1->activeParameter(6), 2.2);
    TS_ASSERT_EQUALS(conv1->nameOfActive(10), "f1.f2.s");
    TS_ASSERT_EQUALS(conv1->activeParameter(10), 3.3);

    TS_ASSERT_EQUALS(conv1->parameterLocalName(0), "a");
    TS_ASSERT_EQUALS(conv1->parameterLocalName(2), "f0.c");
    TS_ASSERT_EQUALS(conv1->parameterLocalName(6), "f1.h");
    TS_ASSERT_EQUALS(conv1->parameterLocalName(10), "f2.s");
  }

  void testResolution() {
    Convolution conv;

    double a = 1.3;
    double h = 3.;
    boost::shared_ptr<ConvolutionTest_Gauss> res =
        boost::make_shared<ConvolutionTest_Gauss>();
    res->setParameter("c", 0);
    res->setParameter("h", h);
    res->setParameter("s", a);

    conv.addFunction(res);

    const int N = 116;
    double x[N], xr[N], out[N], x0 = 0, dx = 0.3;
    double Dx = dx * N;
    for (int i = 0; i < N; i++) {
      x[i] = x0 + i * dx;
      xr[i] = x[i] - x0 - Dx / 2;
    }

    res->function1D(out, xr, N);

    FunctionDomain1DView xView(&x[0], N);
    FunctionValues values(xView);
    // When called with only 1 function attached returns its fourier transform
    conv.function(xView, values);

    // Check that the transform is correct: F( exp(-a*x^2) ) ==
    // sqrt(pi/a)*exp(-(pi*x)^2/a)
    Convolution::HalfComplex hout(values.getPointerToCalculated(0), N);
    double df = 1. / Dx; // this is the x-step of the transformed data
    double pi = acos(0.) * 2;
    double cc = pi * pi * df * df / a;
    for (size_t i = 0; i < hout.size(); i++) {
      TS_ASSERT_DELTA(hout.real(i), h * sqrt(pi / a) * exp(-cc * double(i * i)),
                      1e-7);
    }

    // std::ofstream fres("fres.txt");
    // for(int i=0;i<hout.size();i++)
    //{
    //  double f = df*i;
    //  fres<<f<<' '<<hout.real(i)<<' '<<0
    //    <<' '<<h*sqrt(pi/a)*exp(-pi*pi*f*f/a)<<" 0"<<'\n';
    //}
  }

  void testConvolution() {
    Convolution conv;

    double pi = acos(0.) * 2;
    double c1 = 0.;     // center of the gaussian
    double h1 = 3;      // intensity
    double s1 = pi / 2; // standard deviation
    boost::shared_ptr<ConvolutionTest_Gauss> res =
        boost::make_shared<ConvolutionTest_Gauss>();
    res->setParameter("c", c1);
    res->setParameter("h", h1);
    res->setParameter("s", s1);

    conv.addFunction(res);

    const int N = 116;
    double x[N], x0 = 0, dx = 0.13;
    double Dx = dx * N;
    for (int i = 0; i < N; i++) {
      x[i] = x0 + i * dx;
    }

    double c2 = x0 + Dx / 2;
    double h2 = 10.;
    double s2 = pi / 3;
    boost::shared_ptr<ConvolutionTest_Gauss> fun =
        boost::make_shared<ConvolutionTest_Gauss>();
    fun->setParameter("c", c2);
    fun->setParameter("h", h2);
    fun->setParameter("s", s2);

    conv.addFunction(fun);

    FunctionDomain1DView xView(&x[0], N);
    FunctionValues out(xView);
    conv.function(xView, out);

    // a convolution of two gaussians is a gaussian with h == hp and s == sp
    double sp = s1 * s2 / (s1 + s2);
    double hp = h1 * h2 * sqrt(pi / (s1 + s2));

    // std::cerr<<hp<<' '<<sp<<'\n';
    // std::cerr<<"test.df="<<df<<'\n';
    // std::ofstream fconv("conv.txt");
    for (int i = 0; i < N; i++) {
      double xi = x[i] - c2;
      TS_ASSERT_DELTA(out.getCalculated(i), hp * exp(-sp * xi * xi), 1e-10);
      // fconv<<x[i]<<' '<<out[i]<<' '<< 0/*pi/a/sqrt(2.)*exp(-0.)*/<<'\n';

      // double  f = i*df;
      // fconv<<f<<' '<<h1*h2*pi/sqrt(s1*s2)*exp(-pi*pi*f*f*(1./s1+1./s2))<<"
      // 0"<<'\n';
    }
  }

  /*
   * Convolve a Gausian (resolution) with a Delta-Dirac
   */
  void testConvolvingWithDeltaDirac() {
    Convolution conv;
    // Add resolution function
    double c1 = 0.0; // center of the gaussian
    double h1 = 1.0; // intensity
    double s1 = 1.0; // rate
    auto res = boost::make_shared<ConvolutionTest_Gauss>();
    res->setParameter("c", c1);
    res->setParameter("h", h1);
    res->setParameter("s", s1);
    conv.addFunction(res);
    // Add Delta Dirac function
    double h2 = 1.0;
    auto fun = boost::make_shared<DeltaFunction>();
    fun->setParameter("Height", h2);
    conv.addFunction(fun);
    // Define domains
    const int N = 116;
    double xs[N]; // symmetric range
    double xa[N]; // asymmetric range
    double xm{-4.0}, xMs{4.0}, xMa{8.0};
    double dxs{(xMs - xm) / (N - 1)}, dxa{(xMa - xm) / (N - 1)};
    for (int i = 0; i < N; i++) {
      xs[i] = xm + i * dxs;
      xa[i] = xm + i * dxa;
    }
    // Carry out the convolution
    FunctionDomain1DView ds(&xs[0], N); // symmetric domain
    FunctionDomain1DView da(&xa[0], N); // asymmetric domain
    FunctionValues outs(ds), outa(da);
    conv.function(ds, outs);
    conv.function(da, outa);
    // Check output is the original resolution function
    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(outs.getCalculated(i), h1 * h2 * exp(-s1 * xs[i] * xs[i]),
                      1e-10);
      TS_ASSERT_DELTA(outa.getCalculated(i), h1 * h2 * exp(-s1 * xa[i] * xa[i]),
                      1e-10);
    }
  }

  void testForCategories() {
    Convolution forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "General");
  }

  void testConvolution_fit_resolution() {

    boost::shared_ptr<WorkspaceTester> data =
        boost::make_shared<WorkspaceTester>();
    data->init(1, 100, 100);
    for (size_t i = 0; i < data->blocksize(); i++) {
      data->dataX(0)[i] = -10.0 + 0.2 * double(i);
    }

    boost::shared_ptr<Convolution> conv = boost::make_shared<Convolution>();

    boost::shared_ptr<ConvolutionTest_Gauss> res =
        boost::make_shared<ConvolutionTest_Gauss>();
    res->setParameter("c", 0);
    res->setParameter("h", 1);
    res->setParameter("s", 2);

    conv->addFunction(res);

    boost::shared_ptr<ConvolutionTest_Lorentz> fun =
        boost::make_shared<ConvolutionTest_Lorentz>();
    fun->setParameter("c", 0);
    fun->setParameter("h", 2);
    fun->setParameter("w", 0.5);

    conv->addFunction(fun);

    auto &x = data->dataX(0);
    auto &y = data->dataY(0);

    FunctionDomain1DView xView(&x[0], x.size());
    FunctionValues voigt(xView);
    conv->function(xView, voigt);

    for (size_t i = 0; i < x.size(); i++) {
      y[i] = voigt.getCalculated(i);
    }

    conv->setParameter("f0.h", 0.5);
    conv->setParameter("f0.s", 0.5);
    conv->setParameter("f1.h", 1);
    conv->setParameter("f1.w", 1);

    Algorithms::Fit fit;
    fit.initialize();

    fit.setPropertyValue("Function", conv->asString());
    fit.setProperty("InputWorkspace", data);
    fit.setProperty("WorkspaceIndex", 0);
    fit.execute();

    IFunction_sptr out = fit.getProperty("Function");
    // by default convolution keeps parameters of the resolution (function #0)
    // fixed
    TS_ASSERT_EQUALS(out->getParameter("f0.h"), conv->getParameter("f0.h"));
    TS_ASSERT_EQUALS(out->getParameter("f0.s"), conv->getParameter("f0.s"));
    // fit is not very good
    TS_ASSERT_LESS_THAN(
        0.1, fabs(out->getParameter("f1.w") - conv->getParameter("f1.w")));

    conv->setAttributeValue("FixResolution", false);
    Algorithms::Fit fit1;
    fit1.initialize();
    fit1.setProperty("Function", boost::dynamic_pointer_cast<IFunction>(conv));
    fit1.setProperty("InputWorkspace", data);
    fit1.setProperty("WorkspaceIndex", 0);
    fit1.execute();

    out = fit1.getProperty("Function");
    // resolution parameters change and close to the initial values
    TS_ASSERT_DELTA(out->getParameter("f0.s"), 2.0, 0.00001);
    TS_ASSERT_DELTA(out->getParameter("f1.w"), 0.5, 0.00001);
  }
};

#endif /*CONVOLUTIONTEST_H_*/
