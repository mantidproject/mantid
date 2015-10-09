#ifndef MANTID_API_ILATTICEFUNCTIONTEST_H_
#define MANTID_API_ILATTICEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MantidAPI/ILatticeFunction.h"

using Mantid::API::ILatticeFunction;
using Mantid::Kernel::V3D;

using namespace Mantid::API;

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;

class ILatticeFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ILatticeFunctionTest *createSuite() {
    return new ILatticeFunctionTest();
  }
  static void destroySuite(ILatticeFunctionTest *suite) { delete suite; }

  void testFunctionLatticeIsCalled() {
    /* This test makes sure that functionLattice is called when the correct
     * domain type is supplied. It uses the mock function defined below.
     */
    MockLatticeFunction fn;
    EXPECT_CALL(fn, functionLattice(_, _)).Times(1);

    LatticeDomain domain(getTestHKLs());
    FunctionValues values(domain);

    fn.function(domain, values);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&fn));
  }

  void testFunctionDerivLatticeIsCalled() {
    /* Just as above, this test checks that functionDerivLattice is called
     * correctly. In addition to the mocked function, also MockJacobian is used.
     */
    MockLatticeFunction fn;
    EXPECT_CALL(fn, functionDerivLattice(_, _)).Times(1);

    LatticeDomain domain(getTestHKLs());
    MockJacobian jacobian;

    fn.functionDeriv(domain, jacobian);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&fn));
  }

  void testWrongDomainTypeThrows() {
    // This test makes sure that wrong domain types throw an exception.
    MockLatticeFunction fn;

    MockDomain wrongDomain;
    EXPECT_CALL(wrongDomain, size()).WillRepeatedly(Return(1));
    FunctionValues values(wrongDomain);

    TS_ASSERT_THROWS(fn.function(wrongDomain, values), std::invalid_argument);

    MockJacobian jacobian;
    TS_ASSERT_THROWS(fn.functionDeriv(wrongDomain, jacobian),
                     std::invalid_argument);
  }

private:
  std::vector<V3D> getTestHKLs() {
    std::vector<V3D> hkls;
    hkls.push_back(V3D(1, 1, 0));

    return hkls;
  }

  // Mock function to check whether the correct methods are called
  class MockLatticeFunction : public ILatticeFunction {
  public:
    MockLatticeFunction() : ILatticeFunction() {}
    ~MockLatticeFunction() {}

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_CONST_METHOD2(functionLattice,
                       void(const LatticeDomain &, FunctionValues &));

    MOCK_METHOD2(functionDerivLattice, void(const LatticeDomain &, Jacobian &));

    MOCK_METHOD1(setCrystalSystem, void(const std::string &));
    MOCK_METHOD1(setUnitCell, void(const std::string &));
    MOCK_METHOD1(setUnitCell, void(const Mantid::Geometry::UnitCell &));
    MOCK_CONST_METHOD0(getUnitCell, Mantid::Geometry::UnitCell());
  };

  // Mock jacobian for being able to test derivative calls
  class MockJacobian : public Jacobian {
  public:
    MOCK_METHOD3(set, void(size_t, size_t, double));
    MOCK_METHOD2(get, double(size_t, size_t));
  };

  // Mock domain to simulate a wrong domain type
  class MockDomain : public FunctionDomain {
  public:
    MOCK_CONST_METHOD0(size, size_t());
  };
};

#endif /* MANTID_API_ILATTICEFUNCTIONTEST_H_ */
