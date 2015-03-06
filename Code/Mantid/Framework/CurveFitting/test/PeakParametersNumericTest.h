#ifndef PEAKPARAMETERSNUMERICTEST_H_
#define PEAKPARAMETERSNUMERICTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PeakParametersNumeric.h"
#include "MantidCurveFitting/BackToBackExponential.h"
#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class GaussFun : public PeakParametersNumeric {
public:
  std::pair<double,double> getExtent() const
  {
    double c = getParameter("c");
    double w = getTrueFwhm();
    return std::make_pair(c - 2*w, c + 2*w);
  }

  virtual double getTrueFwhm() const = 0;
};


class GaussLinearW : public GaussFun {
public:
  /// Default constructor.
  GaussLinearW() : GaussFun() {
    declareParameter("h",1.0);
    declareParameter("s",1.0);
    declareParameter("c",0.0);

    defineCentreParameter("c");
    defineHeightParameter("h");
    defineWidthParameter("s",Linear);
  }

  std::string name() const { return "GaussLinearW"; }
  virtual const std::string category() const { return "Peak"; }
  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const
  {
    double h = getParameter("h");
    double s = getParameter("s");
    double c = getParameter("c");

    for(size_t i = 0; i < nData; ++i)
    {
      double tmp = (xValues[i] - c ) / s;
      out[i] = h * exp(- tmp * tmp / 2 );
    }
  }

  double getTrueFwhm() const
  {
    double s = getParameter("s");
    return 2 * sqrt(2.0 * log(2.0)) * s;
  }

protected:
  virtual void functionLocal(double *, const double *, const size_t) const {}
  virtual void functionDerivLocal(API::Jacobian *, const double *,
                                  const size_t) {}
  double expWidth() const;
};

class GaussInverseW : public GaussFun {
public:
  /// Default constructor.
  GaussInverseW() : GaussFun() {
    declareParameter("h",1.0);
    declareParameter("s",1.0);
    declareParameter("c",0.0);

    defineCentreParameter("c");
    defineHeightParameter("h");
    defineWidthParameter("s",Inverse);
  }

  std::string name() const { return "GaussInverseW"; }
  virtual const std::string category() const { return "Peak"; }
  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const
  {
    double h = getParameter("h");
    double s = getParameter("s");
    double c = getParameter("c");

    for(size_t i = 0; i < nData; ++i)
    {
      double tmp = (xValues[i] - c ) * s;
      out[i] = h * exp(- tmp * tmp / 2 );
    }
  }

  double getTrueFwhm() const
  {
    double s = getParameter("s");
    return 2 * sqrt(2.0 * log(2.0)) / s;
  }

protected:
  virtual void functionLocal(double *, const double *, const size_t) const {}
  virtual void functionDerivLocal(API::Jacobian *, const double *,
                                  const size_t) {}
  double expWidth() const;
};

class GaussSquaredW : public GaussFun {
public:
  /// Default constructor.
  GaussSquaredW() : GaussFun() {
    declareParameter("h",1.0);
    declareParameter("s",1.0);
    declareParameter("c",0.0);

    defineCentreParameter("c");
    defineHeightParameter("h");
    defineWidthParameter("s",Square);
  }

  std::string name() const { return "GaussSquaredW"; }
  virtual const std::string category() const { return "Peak"; }
  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const
  {
    double h = getParameter("h");
    double s = getParameter("s");
    double c = getParameter("c");

    for(size_t i = 0; i < nData; ++i)
    {
      double tmp = (xValues[i] - c );
      out[i] = h * exp(- tmp * tmp / 2 / s );
    }
  }

  double getTrueFwhm() const
  {
    double s = getParameter("s");
    return 2 * sqrt(2.0 * log(2.0) * s);
  }

protected:
  virtual void functionLocal(double *, const double *, const size_t) const {}
  virtual void functionDerivLocal(API::Jacobian *, const double *,
                                  const size_t) {}
  double expWidth() const;
};


class PeakParametersNumericTest : public CxxTest::TestSuite {
public:

  void test_GaussLinearW()
  {
    GaussLinearW fun;
    do_test_Gauss(fun,1e-7);
  }

  void test_GaussInverseW()
  {
    GaussInverseW fun;
    do_test_Gauss(fun,1e-4);
  }

  void test_GaussSquaredW()
  {
    GaussSquaredW fun;
    do_test_Gauss(fun,1e-7);
  }

  void test_Back2Back()
  {
    BackToBackExponential fun;
    fun.initialize();
    fun.setParameter("I",1.0);
    fun.setParameter("A",10.0);
    fun.setParameter("B",5.05);
    fun.setParameter("S",0.1);
    double tol = 1e-4;
    TS_ASSERT_DELTA(fun.centre(), 0.0335, tol);
    TS_ASSERT_DELTA(fun.height(), 2.0953, tol);
    TS_ASSERT_DELTA(fun.fwhm(), 0.4027, tol);

    double dh;
    double x = fun.centre() - tol;
    fun.function1D(&dh,&x,1);
    TS_ASSERT( dh < fun.height() );

    x = fun.centre() + tol;
    fun.function1D(&dh,&x,1);
    TS_ASSERT( dh < fun.height() );

    auto range = fun.getExtent();
    double left = 0.0;
    double right = 0.0;
    dh = fun.height() / 2;
    for(double x = range.first; x < range.second; x += tol)
    {
      double y;
      fun.function1D(&y,&x,1);
      if (left == 0.0 && y >= dh )
      {
        left = x;
      }
      if (left != 0.0 && right == 0.0 && y <= dh )
      {
        right = x;
        break;
      }
    }
    TS_ASSERT_DELTA(right - left, fun.fwhm(), tol);

    fun.setCentre(0.0);
    TS_ASSERT_DELTA(fun.centre(), 0.0, tol);
    TS_ASSERT_DELTA(fun.height(), 2.0953, tol);
    TS_ASSERT_DELTA(fun.fwhm(), 0.4027, tol);

    fun.setCentre(-1.0);
    TS_ASSERT_DELTA(fun.centre(), -1.0, tol);
    TS_ASSERT_DELTA(fun.height(), 2.0953, tol);
    TS_ASSERT_DELTA(fun.fwhm(), 0.4027, tol);

    fun.setHeight(1.0);
    TS_ASSERT_DELTA(fun.centre(), -1.0, tol);
    TS_ASSERT_DELTA(fun.height(), 1.0, tol);
    TS_ASSERT_DELTA(fun.fwhm(), 0.4027, tol);
    std::cerr << fun.intensity() << std::endl;

    fun.setHeight(10.0);
    TS_ASSERT_DELTA(fun.centre(), -1.0, tol);
    TS_ASSERT_DELTA(fun.height(), 10.0, tol);
    TS_ASSERT_DELTA(fun.fwhm(), 0.4027, tol);
    std::cerr << fun.intensity() << std::endl;

    fun.setFwhm(1.0);
    TS_ASSERT_DELTA(fun.centre(), -1.0, tol);
    TS_ASSERT_DELTA(fun.height(), 10.0, tol);
    TS_ASSERT_DELTA(fun.fwhm(), 1.0, tol);

  }

private:

  void do_test_Gauss(GaussFun& fun, double tol)
  {
    //std::cerr << fun.centre() << ' ' << fun.height() << ' ' << fun.fwhm() - fun.getTrueFwhm() << std::endl;
    TS_ASSERT_DELTA(fun.centre(), 0.0, tol);
    TS_ASSERT_DELTA(fun.height(), 1.0, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);

    fun.setHeight(2.1);
    TS_ASSERT_DELTA(fun.centre(), 0.0, tol);
    TS_ASSERT_DELTA(fun.height(), 2.1, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);

    fun.setHeight(0.3);
    TS_ASSERT_DELTA(fun.centre(), 0.0, tol);
    TS_ASSERT_DELTA(fun.height(), 0.3, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);

    fun.setCentre(1.3);
    TS_ASSERT_DELTA(fun.centre(), 1.3, tol);
    TS_ASSERT_DELTA(fun.height(), 0.3, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);

    fun.setCentre(-1.3);
    TS_ASSERT_DELTA(fun.centre(), -1.3, tol);
    TS_ASSERT_DELTA(fun.height(), 0.3, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);

    fun.setFwhm(2.0);
    TS_ASSERT_DELTA(fun.centre(), -1.3, tol);
    TS_ASSERT_DELTA(fun.height(), 0.3, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);
    TS_ASSERT_DELTA(fun.fwhm(), 2.0, tol);

    fun.setFwhm(0.001);
    TS_ASSERT_DELTA(fun.centre(), -1.3, tol);
    TS_ASSERT_DELTA(fun.height(), 0.3, tol);
    TS_ASSERT_DELTA(fun.fwhm(), fun.getTrueFwhm(), tol);
    TS_ASSERT_DELTA(fun.fwhm(), 0.001, tol);
  }

};

#endif /*PEAKPARAMETERSNUMERICTEST_H_*/
