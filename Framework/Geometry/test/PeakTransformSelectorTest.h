#ifndef MANTIDAPI_PEAKTRANSFORMSELECTOR_TEST_H_
#define MANTIDAPI_PEAKTRANSFORMSELECTOR_TEST_H_

#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/Crystal/PeakTransformSelector.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid;
using namespace testing;
using boost::regex;

class PeakTransformSelectorTest : public CxxTest::TestSuite {
private:
  /*------------------------------------------------------------
  Mock Peak Transform Factory
  ------------------------------------------------------------*/
  template <int I>
  class MockPeakTransformFactoryType : public PeakTransformFactory {
  private:
    enum { value = I };

  public:
    GCC_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD0(createDefaultTransform, PeakTransform_sptr());
    MOCK_CONST_METHOD2(createTransform,
                       PeakTransform_sptr(const std::string &,
                                          const std::string &));
    GCC_DIAG_ON_SUGGEST_OVERRIDE
  };

  using MockPeakTransformFactory = MockPeakTransformFactoryType<0>;
  using MockPeakTransformFactoryA = MockPeakTransformFactoryType<0>;
  using MockPeakTransformFactoryB = MockPeakTransformFactoryType<1>;

public:
  void test_Constructor() {
    PeakTransformSelector selector;
    TSM_ASSERT_EQUALS("Should have no registered candidates.", 0,
                      selector.numberRegistered());
  }

  void test_RegisterCandiate() {
    MockPeakTransformFactory *pFactory = new MockPeakTransformFactory;
    PeakTransformFactory_sptr mockFactory(pFactory);

    PeakTransformSelector selector;
    selector.registerCandidate(mockFactory);

    TSM_ASSERT_EQUALS("Should have one registered candidate.", 1,
                      selector.numberRegistered());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pFactory));
  }

  void test_MakeChoice_throws_without_candiates() {
    PeakTransformSelector selector;
    TSM_ASSERT_THROWS("Nothing registered so should throw.",
                      selector.makeChoice("H", "K"), std::runtime_error &);
  }

  void test_MakeDefaultChoice_throws_without_candiates() {
    PeakTransformSelector selector;
    TSM_ASSERT_THROWS("Nothing registered so should throw.",
                      selector.makeDefaultChoice(), std::runtime_error &);
  }

  void test_MakeChoice_throws_with_empty_xLabel() {
    MockPeakTransformFactory *pFactory = new MockPeakTransformFactory;
    PeakTransformFactory_sptr mockFactory(pFactory);

    PeakTransformSelector selector;
    selector.registerCandidate(mockFactory);
    TSM_ASSERT_THROWS("xLabel is empty. Should throw.",
                      selector.makeChoice("", "K"), std::invalid_argument &);
  }

  void test_MakeChoice_throws_with_empty_yLabel() {
    MockPeakTransformFactory *pFactory = new MockPeakTransformFactory;
    PeakTransformFactory_sptr mockFactory(pFactory);

    PeakTransformSelector selector;
    selector.registerCandidate(mockFactory);
    TSM_ASSERT_THROWS("yLabel is empty. Should throw.",
                      selector.makeChoice("H", ""), std::invalid_argument &);
  }

  // Check that the selector can identify and return the appropriate factory.
  void test_MakeChoice_Correctly() {
    // Create a Factory that CANNOT handle the transform.
    MockPeakTransformFactoryB *pWrongFactory = new MockPeakTransformFactoryB;
    PeakTransformFactory_sptr wrongFactory(pWrongFactory);
    EXPECT_CALL(*pWrongFactory, createTransform(_, _))
        .WillOnce(Throw(PeakTransformException())); // Will throw when invoked

    // Create a Factory that Can handle the transform.
    MockPeakTransformFactoryA *pRightFactory = new MockPeakTransformFactoryA;
    PeakTransformFactory_sptr rightFactory(pRightFactory);
    PeakTransform_sptr product(new MockPeakTransform); // Product to return
    EXPECT_CALL(*pRightFactory, createTransform(_, _))
        .WillOnce(
            Return(product)); // Will return a PeakTransform without throwing.

    // Set up the selector with candidates.
    PeakTransformSelector selector;
    selector.registerCandidate(wrongFactory);
    selector.registerCandidate(rightFactory);

    // Run the selector
    PeakTransformFactory_sptr selectedFactory = selector.makeChoice("A", "B");

    // Check the outputs and usage.
    TSM_ASSERT("Should not have selected the wrong factory",
               boost::dynamic_pointer_cast<MockPeakTransformFactoryB>(
                   selectedFactory) == nullptr);
    TSM_ASSERT("Should have selected the right factory",
               boost::dynamic_pointer_cast<MockPeakTransformFactoryA>(
                   selectedFactory) != nullptr);
    TS_ASSERT(Mock::VerifyAndClearExpectations(pWrongFactory));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRightFactory));
  }

  // Check that the selector can identify and return the appropriate factory.
  void test_MakeDefaultChoice_Correctly() {
    // Create a Factory that CANNOT handle the transform.
    MockPeakTransformFactoryB *pWrongFactory = new MockPeakTransformFactoryB;
    PeakTransformFactory_sptr wrongFactory(pWrongFactory);
    EXPECT_CALL(*pWrongFactory, createDefaultTransform())
        .WillOnce(Throw(PeakTransformException())); // Will throw when invoked

    // Create a Factory that Can handle the transform.
    MockPeakTransformFactoryA *pRightFactory = new MockPeakTransformFactoryA;
    PeakTransformFactory_sptr rightFactory(pRightFactory);
    PeakTransform_sptr product(new MockPeakTransform); // Product to return
    EXPECT_CALL(*pRightFactory, createDefaultTransform())
        .WillOnce(
            Return(product)); // Will return a PeakTransform without throwing.

    // Set up the selector with candidates.
    PeakTransformSelector selector;
    selector.registerCandidate(wrongFactory);
    selector.registerCandidate(rightFactory);

    // Run the selector
    PeakTransformFactory_sptr selectedFactory = selector.makeDefaultChoice();

    // Check the outputs and usage.
    TSM_ASSERT("Should not have selected the wrong factory",
               boost::dynamic_pointer_cast<MockPeakTransformFactoryB>(
                   selectedFactory) == nullptr);
    TSM_ASSERT("Should have selected the right factory",
               boost::dynamic_pointer_cast<MockPeakTransformFactoryA>(
                   selectedFactory) != nullptr);
    TS_ASSERT(Mock::VerifyAndClearExpectations(pWrongFactory));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRightFactory));
  }

  void test_hasFactoryForTransform_when_doesnt_have_factory_for_transform() {
    // Create a Factory that CANNOT handle the transform.
    MockPeakTransformFactoryB *pWrongFactory = new MockPeakTransformFactoryB;
    PeakTransformFactory_sptr wrongFactory(pWrongFactory);
    EXPECT_CALL(*pWrongFactory, createTransform(_, _))
        .WillOnce(Throw(PeakTransformException())); // Will throw when invoked

    // Set up the selector with candidate.
    PeakTransformSelector selector;
    selector.registerCandidate(wrongFactory);
    // Check results and usage.
    TSM_ASSERT("Should NOT be able to process the transform",
               !selector.hasFactoryForTransform("A", "B"));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pWrongFactory));
  }

  void test_hasFactoryForTransform_when_does_have_factory_for_transform() {
    // Create a Factory that Can handle the transform.
    MockPeakTransformFactoryA *pRightFactory = new MockPeakTransformFactoryA;
    PeakTransformFactory_sptr rightFactory(pRightFactory);
    PeakTransform_sptr product(new MockPeakTransform); // Product to return
    EXPECT_CALL(*pRightFactory, createTransform(_, _))
        .WillOnce(
            Return(product)); // Will return a PeakTransform without throwing.

    // Set up the selector with candidate.
    PeakTransformSelector selector;
    selector.registerCandidate(rightFactory);
    // Check results and usage.
    TSM_ASSERT("Should be able to process the transform",
               selector.hasFactoryForTransform("A", "B"));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pRightFactory));
  }
};

#endif
