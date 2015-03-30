#ifndef MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTIONTEST_H_
#define MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumPawleyFunction.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FunctionFactory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::Kernel;

using ::testing::_;
using ::testing::Mock;

class MockPawleyFunction : public IPawleyFunction {
public:
  MockPawleyFunction() {}
  ~MockPawleyFunction() {}
  // IFunction interface
  MOCK_CONST_METHOD0(name, std::string());
  MOCK_CONST_METHOD2(function, void(const FunctionDomain &, FunctionValues &));

  // IPawleyFunction interface
  MOCK_METHOD1(setCrystalSystem, void(const std::string &));
  MOCK_METHOD1(setProfileFunction, void(const std::string &));
  MOCK_METHOD1(setUnitCell, void(const std::string &));

  MOCK_METHOD3(setPeaks, void(const std::vector<V3D> &, double, double));
  MOCK_METHOD0(clearPeaks, void());
  MOCK_METHOD3(addPeak, void(const V3D &, double, double));
  MOCK_CONST_METHOD0(getPeakCount, size_t(void));
  MOCK_CONST_METHOD1(getPeakFunction, IPeakFunction_sptr(size_t));
  MOCK_CONST_METHOD1(getPeakHKL, V3D(size_t));

  MOCK_METHOD4(setMatrixWorkspace,
               void(MatrixWorkspace_const_sptr, size_t, double, double));

protected:
  void init() { setDecoratedFunction("Gaussian"); }
};

DECLARE_FUNCTION(MockPawleyFunction);

class PoldiSpectrumPawleyFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiSpectrumPawleyFunctionTest *createSuite() {
    return new PoldiSpectrumPawleyFunctionTest();
  }
  static void destroySuite(PoldiSpectrumPawleyFunctionTest *suite) {
    delete suite;
  }

  PoldiSpectrumPawleyFunctionTest() {
    m_detector = boost::shared_ptr<ConfiguredHeliumDetector>(
        new ConfiguredHeliumDetector);
    m_chopper = boost::shared_ptr<MockChopper>(new MockChopper);

    m_spectrum = PoldiSourceSpectrum_sptr(new ConfiguredSpectrum);

    EXPECT_CALL(*m_chopper, distanceFromSample())
        .WillRepeatedly(Return(11800.0));

    EXPECT_CALL(*m_chopper, zeroOffset()).WillRepeatedly(Return(0.15));

    m_instrument = PoldiInstrumentAdapter_sptr(new FakePoldiInstrumentAdapter);
  }

  void test_setDecoratedFunction() {
    PoldiSpectrumPawleyFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(fn.setDecoratedFunction("PawleyFunction"));
    TS_ASSERT_THROWS(fn.setDecoratedFunction("Gaussian"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(fn.setDecoratedFunction("CompositeFunction"),
                     std::invalid_argument);
  }

  void test_getPawleyFunction() {
    PoldiSpectrumPawleyFunction fn;
    fn.initialize();

    TS_ASSERT(!fn.getPawleyFunction());
    fn.setDecoratedFunction("PawleyFunction");

    IFunction_sptr pawleyFn = fn.getDecoratedFunction();
    TS_ASSERT(pawleyFn);
    TS_ASSERT(boost::dynamic_pointer_cast<IPawleyFunction>(pawleyFn));

    TS_ASSERT(fn.getPawleyFunction());
  }

  void test_setMatrixWorkspace() {
    PoldiSpectrumPawleyFunction fn;
    fn.initialize();
    fn.setDecoratedFunction("MockPawleyFunction");

    MatrixWorkspace_const_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace123(4, 10);

    // Make sure the setMatrixWorkspace method can be called directly.
    IPawleyFunction_sptr pFn = fn.getPawleyFunction();
    boost::shared_ptr<MockPawleyFunction> mpFn =
        boost::dynamic_pointer_cast<MockPawleyFunction>(pFn);
    EXPECT_CALL(*mpFn, setMatrixWorkspace(_, _, _, _)).Times(1);

    mpFn->setMatrixWorkspace(ws, 0, 0.0, 0.0);

    /* Make sure the decorated function does not get the matrix workspace
     * so that there are no unit problems (poldi workspaces are in time,
     * calculation needs to be done in d-spacing).
     */
    fn.setMatrixWorkspace(ws, 0, 0.0, 0.0);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&(*mpFn)));
  }

  void test_function1DSpectrum() {
    TestablePoldiSpectrumPawleyFunction fn;
    fn.setDecoratedFunction("PawleyFunction");

    IPawleyFunction_sptr pFn = fn.getPawleyFunction();
    pFn->setProfileFunction("Gaussian");
    pFn->setCrystalSystem("Cubic");
    // Only the first figure matters, because of cubic
    pFn->setUnitCell("5.43122617238802162554 5.431 5.431 90 90 90");
    pFn->addPeak(V3D(4, 2, 2), 0.0027446316797104233, 679.59369981039407842726);

    fn.m_deltaT = 3.0;
    fn.initializeInstrumentParameters(m_instrument);

    std::vector<double> xvalues(500, 1.0);

    FunctionDomain1DSpectrum domain(342, xvalues);
    TS_ASSERT_EQUALS(domain.getWorkspaceIndex(), 342);
    FunctionValues values(domain);
    values.setCalculated(0.0);

    fn.function(domain, values);

    std::vector<double> reference;
    reference.push_back(0.214381692355321);
    reference.push_back(1.4396533098854);
    reference.push_back(7.69011673999647);
    reference.push_back(32.6747845396612);
    reference.push_back(110.432605589092);
    reference.push_back(296.883931458002);
    reference.push_back(634.864220660384);
    reference.push_back(1079.89069118744);
    reference.push_back(1461.11207069126);
    reference.push_back(1572.50503614829);
    reference.push_back(1346.18685763306);
    reference.push_back(916.691981263516);
    reference.push_back(496.502218342172);
    reference.push_back(213.861997764049);
    reference.push_back(73.2741206547921);
    reference.push_back(19.9697293956518);
    reference.push_back(4.32910692237627);
    reference.push_back(0.746498624291666);
    reference.push_back(0.102391587633906);

    for(size_t i = 0; i < reference.size(); ++i) {
        TS_ASSERT_DELTA(values[479 + i] / reference[i], 1.0, 1e-12);
    }
  }

private:
  class TestablePoldiSpectrumPawleyFunction
      : public PoldiSpectrumPawleyFunction {
    friend class PoldiSpectrumPawleyFunctionTest;
  };

  boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
  boost::shared_ptr<MockChopper> m_chopper;
  PoldiSourceSpectrum_sptr m_spectrum;

  PoldiInstrumentAdapter_sptr m_instrument;
};

#endif /* MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTIONTEST_H_ */
