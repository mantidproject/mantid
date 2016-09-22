#ifndef DIFFROTDISCRETECIRCLETEST_H_
#define DIFFROTDISCRETECIRCLETEST_H_

#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Functions/DiffRotDiscreteCircle.h"
#include "MantidCurveFitting/Functions/Gaussian.h"

#include <cmath>

#include <cxxtest/TestSuite.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class DiffRotDiscreteCircleTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiffRotDiscreteCircleTest *createSuite() {
    return new DiffRotDiscreteCircleTest();
  }
  static void destroySuite(DiffRotDiscreteCircleTest *suite) { delete suite; }

  // convolve the elastic part with a resolution function, here a Gaussian
  void testDiffRotDiscreteCircleElastic() {
    // initialize the resolution function
    const double w0 = random_value(-1.0, 1.0);
    const double h = random_value(1.0, 1000.0);
    const double fwhm = random_value(1.0, 100.0);
    boost::shared_ptr<Gaussian> resolution = boost::make_shared<Gaussian>();
    resolution->initialize(); // declare parameters
    resolution->setCentre(w0);
    resolution->setHeight(h);
    resolution->setFwhm(fwhm);

    // initialize the structure factor as the elastic part of
    // DiffRotDiscreteCircle
    const double I = random_value(1.0, 1000.0);
    const double r = random_value(0.3, 9.8);
    const double Q = 0.9;
    const int N = 6;
    boost::shared_ptr<ElasticDiffRotDiscreteCircle> structure_factor(
        new ElasticDiffRotDiscreteCircle());
    structure_factor->setParameter("Height", I);
    structure_factor->setParameter("Radius", r);
    structure_factor->setAttributeValue("Q", Q);
    structure_factor->setAttributeValue("N", N);

    // initialize the convolution function
    Convolution conv;
    conv.addFunction(resolution);
    conv.addFunction(structure_factor);

    // initialize some frequency values centered around zero
    const size_t M = 4001;
    double w[M];
    const double dw = random_value(0.1, 0.5); // bin width
    for (size_t i = 0; i < M; i++)
      w[i] = (static_cast<double>(i) - M / 2) * dw;
    Mantid::API::FunctionDomain1DView xView(&w[0], M);
    Mantid::API::FunctionValues out(xView);

    // convolve
    conv.function(xView, out);

    // Result must be the resolution function multiplied by the intensity of
    // ElasticDiffRotDiscreteCircle
    double scaling = I * structure_factor->HeightPrefactor();
    Mantid::API::FunctionValues out_resolution(xView);
    resolution->function(xView, out_resolution);
    for (size_t i = 0; i < M; i++)
      TS_ASSERT_DELTA(out.getCalculated(i),
                      scaling * out_resolution.getCalculated(i), 1e-3);

  } // testDiffRotDiscreteCircleElastic

  /// check ties between elastic and inelastic parts
  void testDiffRotDiscreteCircleTies() {
    const double I = 2.9;
    const double R = 2.3;
    const double tao = 0.45;
    const double Q = 0.7;
    const int N = 4;
    DiffRotDiscreteCircle func;
    func.init();
    func.setParameter("f1.Intensity", I);
    func.setParameter("f1.Radius", R);
    func.setParameter("f1.Decay", tao);
    func.setAttributeValue("Q", Q);
    func.setAttributeValue("N", N);

    // check values where correctly initialized
    auto ids = boost::dynamic_pointer_cast<InelasticDiffRotDiscreteCircle>(
        func.getFunction(1));
    TS_ASSERT_EQUALS(ids->getParameter("Intensity"), I);
    TS_ASSERT_EQUALS(ids->getParameter("Radius"), R);
    TS_ASSERT_EQUALS(ids->getParameter("Decay"), tao);
    TS_ASSERT_EQUALS(ids->getAttribute("Q").asDouble(), Q);
    TS_ASSERT_EQUALS(ids->getAttribute("Q").asDouble(), Q);

    // check the ties were applied correctly
    func.applyTies(); // elastic parameters are tied to inelastic parameters
    auto eds = boost::dynamic_pointer_cast<ElasticDiffRotDiscreteCircle>(
        func.getFunction(0));
    TS_ASSERT_EQUALS(eds->getParameter("Height"), I);
    TS_ASSERT_EQUALS(eds->getParameter("Radius"), R);
    TS_ASSERT_EQUALS(eds->getAttribute("Q").asDouble(), Q);
  }

  /// check aliases in the composite function
  void testDiffRotDiscreteCircleAliases() {
    const double I = 2.9;
    const double R = 2.3;
    const double tao = 0.45;

    // This should set parameters of the inelastic part
    DiffRotDiscreteCircle func;
    func.init();
    func.setParameter("Intensity", I);
    func.setParameter("Radius", R);
    func.setParameter("Decay", tao);

    // check the parameter of the inelastic part
    auto ifunc = boost::dynamic_pointer_cast<InelasticDiffRotDiscreteCircle>(
        func.getFunction(1));
    TS_ASSERT_EQUALS(ifunc->getParameter("Intensity"), I);
    TS_ASSERT_EQUALS(ifunc->getParameter("Radius"), R);
    TS_ASSERT_EQUALS(ifunc->getParameter("Decay"), tao);

    // check the parameters of the elastic part
    func.applyTies(); // elastic parameters are tied to inelastic parameters
    auto efunc = boost::dynamic_pointer_cast<ElasticDiffRotDiscreteCircle>(
        func.getFunction(0));
    TS_ASSERT_EQUALS(efunc->getParameter("Height"), I);
    TS_ASSERT_EQUALS(efunc->getParameter("Radius"), R);

  } // testDiffRotDiscreteCircleAliases

private:
  /// returns a real value from a uniform distribution
  double random_value(const double &a, const double &b) {
    boost::mt19937 rng;
    boost::uniform_real<double> distribution(a, b);
    return distribution(rng);
  }
};

#endif /* DIFFROTDISCRETECIRCLETEST_H_ */
