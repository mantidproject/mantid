// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

using namespace Mantid::Poldi;
using namespace Mantid::API;

class PoldiSpectrumDomainFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiSpectrumDomainFunctionTest *createSuite() {
    return new PoldiSpectrumDomainFunctionTest();
  }
  static void destroySuite(PoldiSpectrumDomainFunctionTest *suite) {
    delete suite;
  }

  PoldiSpectrumDomainFunctionTest() {
    m_detector = boost::shared_ptr<ConfiguredHeliumDetector>(
        new ConfiguredHeliumDetector);
    m_chopper = boost::make_shared<MockChopper>();

    m_spectrum = PoldiSourceSpectrum_sptr(new ConfiguredSpectrum);

    EXPECT_CALL(*m_chopper, distanceFromSample())
        .WillRepeatedly(Return(11800.0));

    EXPECT_CALL(*m_chopper, zeroOffset()).WillRepeatedly(Return(0.15));

    m_instrument = PoldiInstrumentAdapter_sptr(new FakePoldiInstrumentAdapter);
  }

  void testInit() {
    PoldiSpectrumDomainFunction function;
    TS_ASSERT_THROWS_NOTHING(function.initialize());

    // Function has no parameters/attributes
    TS_ASSERT_EQUALS(function.nParams(), 0);
    TS_ASSERT_EQUALS(function.nAttributes(), 0);
  }

  void testProfileFunctionAttribute() {
    PoldiSpectrumDomainFunction function;
    function.initialize();

    TS_ASSERT_EQUALS(function.nParams(), 0);

    TS_ASSERT_THROWS_NOTHING(function.setDecoratedFunction("Gaussian"));

    // Make sure the parameters are exposed correctly
    IFunction_sptr gaussian =
        FunctionFactory::Instance().createFunction("Gaussian");
    TS_ASSERT_EQUALS(function.nParams(), gaussian->nParams());
    for (size_t i = 0; i < gaussian->nParams(); ++i) {
      TS_ASSERT_EQUALS(function.parameterName(i), gaussian->parameterName(i));
    }

    TS_ASSERT_THROWS_NOTHING(function.setDecoratedFunction("DeltaFunction"));
    IFunction_sptr delta =
        FunctionFactory::Instance().createFunction("DeltaFunction");
    TS_ASSERT_EQUALS(function.nParams(), delta->nParams());
    for (size_t i = 0; i < delta->nParams(); ++i) {
      TS_ASSERT_EQUALS(function.parameterName(i), delta->parameterName(i));
    }
  }

  void testChopperSlitOffsets() {
    TestablePoldiSpectrumDomainFunction function;

    std::vector<double> offsets = function.getChopperSlitOffsets(m_chopper);

    for (size_t i = 0; i < offsets.size(); ++i) {
      TS_ASSERT_EQUALS(offsets[i],
                       m_chopper->slitTimes()[i] + m_chopper->zeroOffset());
    }
  }

  void testInitializeFromInstrument() {
    TestablePoldiSpectrumDomainFunction function;
    function.initializeInstrumentParameters(m_instrument);

    TS_ASSERT_EQUALS(function.m_chopperSlitOffsets.size(),
                     m_chopper->slitPositions().size());
  }

  void testFunction() {
    TestablePoldiSpectrumDomainFunction function;
    function.initialize();
    function.setDecoratedFunction("Gaussian");
    function.setParameter("Height", 679.59369981039407842726); // 1.9854805);
    function.setParameter("Sigma",
                          0.0027446316797104233 / (2.0 * sqrt(2.0 * M_LN2)));
    function.setParameter("PeakCentre", 1.1086444);

    function.m_deltaT = 3.0;
    function.initializeInstrumentParameters(m_instrument);

    std::vector<double> xvalues(500, 1.0);

    FunctionDomain1DSpectrum domain(342, xvalues);
    TS_ASSERT_EQUALS(domain.getWorkspaceIndex(), 342);
    FunctionValues values(domain);
    values.setCalculated(0.0);

    function.function(domain, values);

    std::array<double, 19> reference{
        {0.214381692355321, 1.4396533098854, 7.69011673999647, 32.6747845396612,
         110.432605589092, 296.883931458002, 634.864220660384, 1079.89069118744,
         1461.11207069126, 1572.50503614829, 1346.18685763306, 916.691981263516,
         496.502218342172, 213.861997764049, 73.2741206547921, 19.9697293956518,
         4.32910692237627, 0.746498624291666, 0.102391587633906}};

    for (size_t i = 0; i < reference.size(); ++i) {
      TS_ASSERT_DELTA(values[479 + i] / reference[i], 1.0, 1e-14);
    }
  }

  void testFunctionDeriv() {
    TestablePoldiSpectrumDomainFunction function;
    function.initialize();
    function.setDecoratedFunction("Gaussian");
    function.setParameter("Height", 679.59369981039407842726); // 1.9854805);
    function.setParameter("Sigma",
                          0.0027446316797104233 / (2.0 * sqrt(2.0 * M_LN2)));
    function.setParameter("PeakCentre", 1.1086444);

    function.m_deltaT = 3.0;
    function.initializeInstrumentParameters(m_instrument);

    std::vector<double> xvalues(500, 1.0);
    FunctionDomain1DSpectrum domain(342, xvalues);
    TS_ASSERT_EQUALS(domain.getWorkspaceIndex(), 342);
    Mantid::CurveFitting::Jacobian jacobian(500, 3);

    function.functionDeriv(domain, jacobian);

    std::array<double, 19> reference{
        {0.214381692355321, 1.4396533098854, 7.69011673999647, 32.6747845396612,
         110.432605589092, 296.883931458002, 634.864220660384, 1079.89069118744,
         1461.11207069126, 1572.50503614829, 1346.18685763306, 916.691981263516,
         496.502218342172, 213.861997764049, 73.2741206547921, 19.9697293956518,
         4.32910692237627, 0.746498624291666, 0.102391587633906}};

    for (size_t i = 0; i < reference.size(); ++i) {
      TS_ASSERT_DELTA((jacobian.get(479 + i, 0)) /
                          (reference[i] / 679.59369981039407842726),
                      1.0, 1e-14);
    }
  }

  void testAccessThroughBasePointer() {
    TestablePoldiSpectrumDomainFunction *function =
        new TestablePoldiSpectrumDomainFunction();
    function->initialize();
    function->setDecoratedFunction("Gaussian");
    function->setParameter("Height", 1.9854805);
    function->setParameter("Sigma",
                           0.0027446316797104233 / (2.0 * sqrt(2.0 * M_LN2)));
    function->setParameter("PeakCentre", 1.1086444);

    function->m_deltaT = 3.0;
    function->initializeInstrumentParameters(m_instrument);

    TS_ASSERT_EQUALS(function->getParameter("PeakCentre"), 1.1086444);

    MultiDomainFunction mdf;
    mdf.addFunction(IFunction_sptr(dynamic_cast<IFunction *>(function)));

    TS_ASSERT_EQUALS(
        static_cast<IFunction *>(&mdf)->getParameter("f0.PeakCentre"),
        1.1086444);
  }

  void testLocalJacobianConstruction() {
    TS_ASSERT_THROWS_NOTHING(LocalJacobian localJacobian(0, 0));
    TS_ASSERT_THROWS_NOTHING(LocalJacobian localJacobian(0, 10));
    TS_ASSERT_THROWS_NOTHING(LocalJacobian localJacobian(10, 0));
    TS_ASSERT_THROWS_NOTHING(LocalJacobian localJacobian(10, 10));
  }

  void testLocalJacobianGetSet() {
    /* These checks also verify that the protected methods
     * getRaw, index and safeIndex work as expected.
     */
    LocalJacobian localJacobian(20, 3);

    for (size_t y = 0; y < 20; ++y) {
      for (size_t p = 0; p < 3; ++p) {
        double value = static_cast<double>(y * p);
        TS_ASSERT_THROWS_NOTHING(localJacobian.set(y, p, value));
        TS_ASSERT_THROWS_NOTHING(localJacobian.get(y, p));

        TS_ASSERT_EQUALS(localJacobian.get(y, p), value);
      }
    }

    TS_ASSERT_THROWS(localJacobian.set(20, 3, 30.0), const std::out_of_range &);
    TS_ASSERT_THROWS(localJacobian.set(10, 4, 30.0), const std::out_of_range &);

    TS_ASSERT_THROWS(localJacobian.get(20, 3), const std::out_of_range &);
    TS_ASSERT_THROWS(localJacobian.get(10, 4), const std::out_of_range &);
  }

  void testLocalJacobianRawValues() {
    LocalJacobian writeAdapter(3, 1);

    double *rawJacobianWrite = writeAdapter.rawValues();
    for (size_t i = 0; i < 3; ++i) {
      *(rawJacobianWrite + i) = static_cast<double>(i + 1);
    }

    TS_ASSERT_EQUALS(writeAdapter.get(0, 0), 1.0);
    TS_ASSERT_EQUALS(writeAdapter.get(1, 0), 2.0);
    TS_ASSERT_EQUALS(writeAdapter.get(2, 0), 3.0);

    LocalJacobian readAdapter(3, 1);
    readAdapter.set(0, 0, 1.0);
    readAdapter.set(1, 0, 2.0);
    readAdapter.set(2, 0, 3.0);

    double *rawJacobianRead = readAdapter.rawValues();
    for (size_t i = 0; i < 3; ++i) {
      TS_ASSERT_EQUALS(*(rawJacobianRead + i), static_cast<double>(i + 1));
    }
  }

  /*
   * This test must be re-enabled, when
   * https://github.com/mantidproject/mantid/issues/10340
   * (orginally trac issue #9497) is fixed, then it will pass.
   *
   * As of 2016/06/22 thre is still an issue with the precision in the
   * conversion to/from strings
   */
  void ___testCreateInitialized() {
    IFunction_sptr function =
        FunctionFactory::Instance().createFunction("Gaussian");
    function->initialize();
    function->setParameter(0, 1.23456);
    function->setParameter(1, 1.234567);
    function->setParameter(2, 0.01234567);

    IFunction_sptr clone = function->clone();

    // passes, Parameter 0 has less than 7 significant digits
    TS_ASSERT_EQUALS(function->getParameter(0), clone->getParameter(0));

    // fails, Parameter 1 has more than 7 significant digits
    TS_ASSERT_EQUALS(function->getParameter(1), clone->getParameter(1));

    // fails, Parameter 2 has more than 7 significant digits
    TS_ASSERT_EQUALS(function->getParameter(2), clone->getParameter(2));
  }

private:
  class TestablePoldiSpectrumDomainFunction : PoldiSpectrumDomainFunction {
    friend class PoldiSpectrumDomainFunctionTest;

    TestablePoldiSpectrumDomainFunction() : PoldiSpectrumDomainFunction() {}
  };

  boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
  boost::shared_ptr<MockChopper> m_chopper;
  PoldiSourceSpectrum_sptr m_spectrum;

  PoldiInstrumentAdapter_sptr m_instrument;
};

#endif /* MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_ */
