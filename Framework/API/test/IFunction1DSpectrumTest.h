// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IFUNCTION1DSPECTRUMTEST_H_
#define MANTID_API_IFUNCTION1DSPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/ParamFunction.h"

using namespace Mantid::API;

class IFunction1DSpectrumTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IFunction1DSpectrumTest *createSuite() {
    return new IFunction1DSpectrumTest();
  }
  static void destroySuite(IFunction1DSpectrumTest *suite) { delete suite; }

  void testConstruction() {
    TS_ASSERT_THROWS_NOTHING(ConcreteFunction1DSpectrum function1DSpectrum);
  }

  void testFunctionCorrectDomain() {
    std::vector<double> xValues;
    xValues.push_back(1.0);
    xValues.push_back(2.0);
    xValues.push_back(3.0);
    xValues.push_back(4.0);
    xValues.push_back(5.0);

    FunctionDomain1DSpectrum domain(0, xValues);
    FunctionValues values(domain);

    IFunction_sptr fun(new ConcreteFunction1DSpectrum);

    TS_ASSERT_THROWS_NOTHING(fun->function(domain, values));

    TS_ASSERT_EQUALS(values[0], 1.0);
    TS_ASSERT_EQUALS(values[1], 2.0);
  }

  void testFunctionIncorrectDomain() {
    std::vector<double> xValues;
    xValues.push_back(1.0);
    xValues.push_back(2.0);
    xValues.push_back(3.0);
    xValues.push_back(4.0);
    xValues.push_back(5.0);

    FunctionDomain1DVector domain(xValues);
    FunctionValues values(domain);

    IFunction_sptr fun(new ConcreteFunction1DSpectrum);

    TS_ASSERT_THROWS(fun->function(domain, values), const std::invalid_argument &);
  }

private:
  class ConcreteFunction1DSpectrum : public virtual ParamFunction,
                                     public virtual IFunction1DSpectrum {
    friend class IFunction1DSpectrumTest;

  public:
    std::string name() const override { return "ConcreteFunction1DSpectrum"; }

    void function1DSpectrum(const FunctionDomain1DSpectrum &domain,
                            FunctionValues &values) const override {
      for (size_t i = 0; i < domain.size(); ++i) {
        values.addToCalculated(i, domain[i]);
      }
    }
  };
};

#endif /* MANTID_API_IFUNCTION1DSPECTRUMTEST_H_ */
