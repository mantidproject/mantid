#ifndef FUNCTIONSTESTHELPER_H_
#define FUNCTIONSTESTHELPER_H_

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid;
using namespace Mantid::API;

/*
 * Gaussian fit function
 */
class Gauss : public IPeakFunction {
public:
    Gauss() {
        declareParameter("c");
        declareParameter("h", 1.);
        declareParameter("s", 1.);
    }

    std::string name() const override { return "Gauss"; }

    void functionLocal(double *out, const double *xValues,
                       const size_t nData) const override {
        double c = getParameter("c");
        double h = getParameter("h");
        double w = getParameter("s");
        for (size_t i = 0; i < nData; i++) {
            double x = xValues[i] - c;
            out[i] = h * exp(-0.5 * x * x * w);
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
            double e = h * exp(-0.5 * x * x * w);
            out->set(static_cast<int>(i), 0, x * h * e * w);
            out->set(static_cast<int>(i), 1, e);
            out->set(static_cast<int>(i), 2, -0.5 * x * x * h * e);
        }
    }

    double centre() const override { return getParameter(0); }

    double height() const override { return getParameter(1); }

    double fwhm() const override { return getParameter(2); }

    void setCentre(const double c) override { setParameter(0, c); }
    void setHeight(const double h) override { setParameter(1, h); }

    void setFwhm(const double w) override { setParameter(2, w); }
};

/*
 * Linear fit function
 */
class Linear : public ParamFunction, public IFunction1D {
public:
    Linear() {
        declareParameter("a");
        declareParameter("b");
    }

    std::string name() const override { return "Linear"; }

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
            out->set(static_cast<int>(i), 0, 1.);
            out->set(static_cast<int>(i), 1, xValues[i]);
        }
    }
};

/*
 * Cubic fit function
 */
class Cubic : public ParamFunction, public IFunction1D {
public:
    Cubic() {
        declareParameter("c0");
        declareParameter("c1");
        declareParameter("c2");
        declareParameter("c3");
    }

    std::string name() const override { return "Cubic"; }

    void function1D(double *out, const double *xValues,
                    const size_t nData) const override {
        double c0 = getParameter("c0");
        double c1 = getParameter("c1");
        double c2 = getParameter("c2");
        double c3 = getParameter("c3");
        for (size_t i = 0; i < nData; i++) {
            double x = xValues[i];
            out[i] = c0 + x * (c1 + x * (c2 + x * c3));
        }
    }
    void functionDeriv1D(Jacobian *out, const double *xValues,
                         const size_t nData) override {
        for (size_t i = 0; i < nData; i++) {
            double x = xValues[i];
            out->set(static_cast<int>(i), 0, 1.);
            out->set(static_cast<int>(i), 1, x);
            out->set(static_cast<int>(i), 2, x * x);
            out->set(static_cast<int>(i), 3, x * x * x);
        }
    }
};

#endif /* FUNCTIONSHELPER */