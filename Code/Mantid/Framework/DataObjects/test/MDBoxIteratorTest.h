#ifndef MANTID_DATAOBJECTS_MDBOXITERATORTEST_H_
#define MANTID_DATAOBJECTS_MDBOXITERATORTEST_H_

#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <gmock/gmock.h>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Geometry::MDImplicitFunction;
using Mantid::Geometry::MDImplicitFunction;
using Mantid::Geometry::MDPlane;
using Mantid::Geometry::MDBoxImplicitFunction;


class MDBoxIteratorTest : public CxxTest::TestSuite
{
public:
  typedef MDGridBox<MDLeanEvent<1>,1> gbox_t;
  typedef MDBoxBase<MDLeanEvent<1>,1> ibox_t;

  //--------------------------------------------------------------------------------------
  /** Make a gridded box with this structure:
              Names                                   Width of each box

               A                                        64
               |
      B0 -- B1 -------- B2 ------------ B3              16
      |                 |
    C00-3        C20 C21 C22 C23                         4
                      |
                  D210 D211 D212 D213                    1

   */
  gbox_t *A;
  ibox_t *B1, *B3;
  gbox_t *B0, *B2;
  ibox_t *C00, *C01, *C02, *C03;
  ibox_t *C20,       *C22, *C23;
  gbox_t *C21;
  ibox_t *D210, *D211, *D212, *D213;
  MDBoxIterator<MDLeanEvent<1>,1> * it;
  void setUp()
  {
    // Top level grid box
    A = MDEventsTestHelper::makeMDGridBox<1>(4, 1, 0.0, 64.0);
    A->splitContents(0); // Split B0 into C00 to C03
    A->splitContents(2); // Split B2 into C20 to C23
    B0 = dynamic_cast<gbox_t *>(A->getChild(0));
    B2 = dynamic_cast<gbox_t *>(A->getChild(2));
    B2->splitContents(1); // Split C21 into D210 to D212

    B1 = dynamic_cast<ibox_t *>(A->getChild(1));
    B3 = dynamic_cast<ibox_t *>(A->getChild(3));
    TS_ASSERT_EQUALS( B1, dynamic_cast<ibox_t *>(A->getChild(1)));
    C00 = dynamic_cast<ibox_t *>(B0->getChild(0));
    C01 = dynamic_cast<ibox_t *>(B0->getChild(1));
    C02 = dynamic_cast<ibox_t *>(B0->getChild(2));
    C03 = dynamic_cast<ibox_t *>(B0->getChild(3));
    C20 = dynamic_cast<ibox_t *>(B2->getChild(0));
    C21 = dynamic_cast<gbox_t *>(B2->getChild(1));
    C22 = dynamic_cast<ibox_t *>(B2->getChild(2));
    C23 = dynamic_cast<ibox_t *>(B2->getChild(3));
    D210 = dynamic_cast<ibox_t *>(C21->getChild(0));
    D211 = dynamic_cast<ibox_t *>(C21->getChild(1));
    D212 = dynamic_cast<ibox_t *>(C21->getChild(2));
    D213 = dynamic_cast<ibox_t *>(C21->getChild(3));
  }
  
  void tearDown()
  {
    delete A->getBoxController();
    delete A;
  }

  //--------------------------------------------------------------------------------------
  void test_ctor_with_null_box_fails()
  {
    typedef MDBoxIterator<MDLeanEvent<1>,1> boxit_t;
    TS_ASSERT_THROWS_ANYTHING( new boxit_t(NULL, 10, false); );
  }

  //--------------------------------------------------------------------------------------
  /** Increment the iterator and return true if the next box is the expected one*/
  bool nextIs(MDBoxIterator<MDLeanEvent<1>,1> * it, ibox_t * expected)
  {
    // std::cout << it->getBox()->getExtentsStr() << std::endl;
    if (!it->next()) return false;
    if (it->getBox() != expected) return false;
    return true;
  }

  void test_iterator_basic()
  {
    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false);

    // Start with the top one
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B0) );
    TS_ASSERT( nextIs(it, C00) );
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    // Calling next again does not cause problems.
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_depth_limit_1()
  {
    // Limit depth to 1 (the B level)
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 1, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B0) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_depth_limit_0()
  {
    // Limit depth to 0 (the A level)
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 0, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_starting_deeper()
  {
    // Start at a depth of 1 (on B0)
    it = new MDBoxIterator<MDLeanEvent<1>,1>(B0, 20, false);
    TS_ASSERT_EQUALS( it->getBox(), B0);
    TS_ASSERT( nextIs(it, C00) );
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_leaf_only()
  {
    // Leaf-only iterator = skips anything with children
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true);

    // This is the first leaf node
    TS_ASSERT_EQUALS( it->getBox(), C00);
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_leaf_only_depth_2()
  {
    // A node is considered a 'leaf' if it is at maxDepth
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 2, true);

    // This is the first leaf node
    TS_ASSERT_EQUALS( it->getBox(), C00);
    TS_ASSERT( nextIs(it, C01) );
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, C21) ); // This is now a 'leaf' due to the maxDepth
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_leaf_only_depth_1()
  {
    // A node is considered a 'leaf' if it is at maxDepth
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 1, true);
    // This is the first leaf node
    TS_ASSERT_EQUALS( it->getBox(), B0);
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, B3) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_leaf_only_depth_0()
  {
    // A node is considered a 'leaf' if it is at maxDepth
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 0, true);
    // This is the ONLY leaf node
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_leaf_only_starting_deeper()
  {
    //Now we start at B2 and look at only leaves
    it = new MDBoxIterator<MDLeanEvent<1>,1>(B2, 10, true);
    TS_ASSERT_EQUALS( it->getBox(), C20);
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
    
    delete it;
  }

  void test_leaf_only_starting_deeper_depth_limited()
  {
    //Now we start at B2 and look at only leaves up to depth 2
    it = new MDBoxIterator<MDLeanEvent<1>,1>(B2, 2, true);
    TS_ASSERT_EQUALS( it->getBox(), C20);
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  //--------------------------------------------------------------------------------------
  /** If you have just a MD Box, there is only one return from the iterator,
   * and it doesn't crash or anything ;)
   */
  void test_iterator_just_one_box()
  {
    // Top level grid box
    ibox_t * A = MDEventsTestHelper::makeMDBox1();
    MDBoxIterator<MDLeanEvent<1>,1> * it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    BoxController *const bc = A->getBoxController();
    delete bc;
    delete it;
    delete A;
  }

  //--------------------------------------------------------------------------------------
  /** Get the inner data from an MDBox
   */
  void test_iterator_getInnerData()
  {
    // Make a MDBox with 10 events
    ibox_t * A = MDEventsTestHelper::makeMDBox1();
    MDEventsTestHelper::feedMDBox<1>(A,1,10, 0.5, 1.0);
    MDBoxIterator<MDLeanEvent<1>,1> * it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false);
    TS_ASSERT_EQUALS( it->getBox(), A);
    // It has some events!
    TS_ASSERT_EQUALS( it->getNumEvents(), 10);
    // Check inner data
    for (size_t i=0; i<10; i++)
    {
      TS_ASSERT_DELTA( it->getInnerSignal(i), 1.0, 1e-6);
      TS_ASSERT_DELTA( it->getInnerError(i), 1.0, 1e-6);
      TS_ASSERT_DELTA( it->getInnerRunIndex(i), 0, 0);
      TS_ASSERT_DELTA( it->getInnerDetectorID(i), 0, 0);
      TS_ASSERT_DELTA( it->getInnerPosition(i,0), 0.5 + double(i), 1e-6);
    }
    // Other functions that go back to the box
    TS_ASSERT_DELTA( it->getCenter()[0], 5.0, 1e-5);

    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    BoxController *const bc = A->getBoxController();
    delete bc;
    delete it;
    delete A;
  }
  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_above11()
  {
    // Implicit function = only boxes with x > 5.0
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {11.0};
    func->addPlane( MDPlane(1, normal, origin) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false, func);
    delete func;

    // Start with the top one
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B0) );
    // C00-C01 are outside the range
    TS_ASSERT( nextIs(it, C02) );
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_above11_leafOnly()
  {
    // Implicit function = only boxes with x > 5.0
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {11.0};
    func->addPlane( MDPlane(1, normal, origin) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true, func);
    delete func;

    // C00-C01 are outside the range, so the first one is C02
    TS_ASSERT_EQUALS( it->getBox(), C02);
    TS_ASSERT( nextIs(it, C03) );
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }


  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_above17()
  {
    // Implicit function = only boxes with x > 17.0
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {17.0};
    func->addPlane( MDPlane(1, normal, origin) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false, func);
    delete func;

    // Start with the top one
    TS_ASSERT_EQUALS( it->getBox(), A);
    // B0 (and all children) are outside the range
    TS_ASSERT( nextIs(it, B1) );
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, C20) );
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, D210) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    TS_ASSERT( nextIs(it, D213) );
    TS_ASSERT( nextIs(it, C22) );
    TS_ASSERT( nextIs(it, C23) );
    TS_ASSERT( nextIs(it, B3) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }


  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_between_37_and_39()
  {
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {37.1f};
    func->addPlane( MDPlane(1, normal, origin) );
    coord_t normal2[1] = {-1.0}; coord_t origin2[1] = {38.9f};
    func->addPlane( MDPlane(1, normal2, origin2) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false, func);
    delete func;

    // Go down to the only two leaf boxes that are in range
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( nextIs(it, B2) );
    TS_ASSERT( nextIs(it, C21) );
    TS_ASSERT( nextIs(it, D211) );
    TS_ASSERT( nextIs(it, D212) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_between_37_and_39_leafOnly()
  {
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {37.1f};
    func->addPlane( MDPlane(1, normal, origin) );
    coord_t normal2[1] = {-1.0}; coord_t origin2[1] = {38.9f};
    func->addPlane( MDPlane(1, normal2, origin2) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true, func);
    delete func;

    // Only two leaf boxes are in range
    TS_ASSERT_EQUALS( it->getBox(), D211);
    TS_ASSERT( nextIs(it, D212) );
    // No more!
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_noBoxInRange()
  {
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {234};
    func->addPlane( MDPlane(1, normal, origin) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, false, func);
    delete func;

    // Returns the first box but that's it
    TS_ASSERT_EQUALS( it->getBox(), A);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );
    
    delete it;
  }

  //--------------------------------------------------------------------------------------
  void test_iterator_withImplicitFunction_noBoxInRange_leafOnly()
  {
    MDImplicitFunction * func = new MDImplicitFunction();
    coord_t normal[1] = {+1.0}; coord_t origin[1] = {234};
    func->addPlane( MDPlane(1, normal, origin) );

    // Create an iterator
    it = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true, func);
    delete func;
    
    // Nothing in the iterator!
    TS_ASSERT_EQUALS( it->getDataSize(), 0);
    TS_ASSERT( !it->next() );
    TS_ASSERT( !it->next() );

    delete it;
  }

  void test_getIsMasked()
  {
    //Mock MDBox. Only one method of interest to the mocking.
    class MockMDBox : public MDBox<MDLeanEvent<2>, 2>
    {
        API::BoxController *const pBC;
    public:
        MockMDBox():
            MDBox<MDLeanEvent<2>, 2>(new API::BoxController(2)),
            pBC(MDBox<MDLeanEvent<2>, 2>::getBoxController())

        {}
        MOCK_CONST_METHOD0(getIsMasked, bool());
       ~MockMDBox()
        {delete pBC;}

    };

    MockMDBox mockBox;

    MDBoxIterator<MDLeanEvent<2>, 2> it(&mockBox, 1, true);

    //All that we want to test is that iterator::getIsMasked calls MDBoxBase::getIsMasked
    EXPECT_CALL(mockBox, getIsMasked()).Times(1);
    it.getIsMasked();

    TSM_ASSERT("Iterator does not use boxes as expected", testing::Mock::VerifyAndClearExpectations(&mockBox));
  }

  void test_skip_masked_detectors()
  {
    MDBoxIterator<MDLeanEvent<1>,1>* setupIterator = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true);
  
    //mask box 0, unmask 1 and Mask box 2. From box 3 onwards, boxes will be unmasked.
    setupIterator->getBox()->mask();
    setupIterator->next(1);
    setupIterator->getBox()->unmask(); 
    setupIterator->next(1);
    setupIterator->getBox()->mask();
    setupIterator->next(1);

    MDBoxIterator<MDLeanEvent<1>,1>* evaluationIterator = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true);
    TS_ASSERT_THROWS_NOTHING(evaluationIterator->next());
    TSM_ASSERT_EQUALS("Should have skipped to the first non-masked box", 1, evaluationIterator->getPosition());
    TS_ASSERT_THROWS_NOTHING(evaluationIterator->next());
    TSM_ASSERT_EQUALS("Should have skipped to the second non-masked box", 3, evaluationIterator->getPosition());
    TSM_ASSERT("The last box should be masked", !evaluationIterator->getIsMasked());

    delete setupIterator;
    delete evaluationIterator;
  }

  void test_no_skipping_policy()
  {
    MDBoxIterator<MDLeanEvent<1>,1>* setupIterator = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true);
  
    //mask box 0, unmask 1 and Mask box 2. From box 3 onwards, boxes will be unmasked.
    setupIterator->getBox()->mask();
    setupIterator->next(1);
    setupIterator->getBox()->unmask(); 
    setupIterator->next(1);
    setupIterator->getBox()->mask();
    setupIterator->next(1);

    MDBoxIterator<MDLeanEvent<1>,1>* evaluationIterator = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true, new SkipNothing); //Using skip nothing policy.
    TS_ASSERT_THROWS_NOTHING(evaluationIterator->next());
    TSM_ASSERT_EQUALS("Should NOT have skipped to the first box", 1, evaluationIterator->getPosition());
    TS_ASSERT_THROWS_NOTHING(evaluationIterator->next());
    TSM_ASSERT_EQUALS("Should NOT have skipped to the second box", 2, evaluationIterator->getPosition());
    TS_ASSERT_THROWS_NOTHING(evaluationIterator->next());
    TSM_ASSERT_EQUALS("Should NOT have skipped to the third box", 3, evaluationIterator->getPosition());

    delete setupIterator;
    delete evaluationIterator;
  }

  void test_custom_skipping_policy()
  {
    /// Mock Skipping Policy Type to inject.
    class MockSkippingPolicy : public SkippingPolicy
    {
    public:
      MOCK_CONST_METHOD0(keepGoing, bool());
      MOCK_METHOD0(Die, void());
      ~MockSkippingPolicy(){Die();}
    };

    MockSkippingPolicy* mockPolicy = new MockSkippingPolicy;
    MDBoxIterator<MDLeanEvent<1>,1>* evaluationIterator = new MDBoxIterator<MDLeanEvent<1>,1>(A, 20, true, mockPolicy); //Using custom policy

    EXPECT_CALL(*mockPolicy, Die()).Times(1); //Should call destructor automatically within MDBoxIterator
    EXPECT_CALL(*mockPolicy, keepGoing()).Times(static_cast<int>(evaluationIterator->getDataSize())); //Should apply test 

    while(evaluationIterator->next()) //Keep calling next while true. Will iterate through all boxes.
    {
    }
    delete evaluationIterator;

    TSM_ASSERT("Has not used SkippingPolicy as expected.", testing::Mock::VerifyAndClearExpectations(mockPolicy));
  }
};

class MDBoxIteratorTestPerformance : public CxxTest::TestSuite
{
  MDGridBox<MDLeanEvent<3>,3> * top;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDBoxIteratorTestPerformance *createSuite() { return new MDBoxIteratorTestPerformance(); }
  static void destroySuite( MDBoxIteratorTestPerformance *suite ) { delete suite; }


  MDBoxIteratorTestPerformance()
  {
    // 1968876 boxes in this. Top box is 5*5*5
    top = MDEventsTestHelper::makeRecursiveMDGridBox<3>(5, 2);
  }

public:
  // ---------------------------------------------------------------
  /** Make a simple iterator that will go through all the boxes */
  void do_test_iterator(bool leafOnly, bool ImplicitFunction, size_t expected)
  {
    // Count the top level box.
    size_t counter = 1;
    MDBoxBase<MDLeanEvent<3>,3> * box = NULL;

    MDBoxImplicitFunction * function = NULL;
    if (ImplicitFunction)
    {
      std::vector<coord_t> min(3, 2.001f);
      std::vector<coord_t> max(3, 2.999f);
      function = new MDBoxImplicitFunction(min, max);
    }

    MDBoxIterator<MDLeanEvent<3>,3> it(top, 20, leafOnly, new SkipNothing, function);

    // Count all of them
    while (it.next())
    {
      box = it.getBox();
      counter++;
    }
    TS_ASSERT( box );
    TS_ASSERT_EQUALS( counter, expected );
  }

  void test_iterator()
  {
    do_test_iterator(false, false, 125*125*125 + 125*125 + 125 + 1);
  }

  void test_iterator_leafOnly()
  {
    do_test_iterator(true, false, 125*125*125);
  }

  void test_iterator_withImplicitFunction()
  {
    do_test_iterator(false, true, 1 + 125*125 + 125 + 1);
  }

  void test_iterator_withImplicitFunction_leafOnly()
  {
    do_test_iterator(true, true, 125*125);
  }

  // ---------------------------------------------------------------
  /** This iterator will also take the boxes and fill
   * a vector with them; this is a full speed comparison to getBoxes()
   * which directly returns that vector, if that happens to be what you need.*/
  void do_test_iterator_that_fills_a_vector(bool leafOnly)
  {
    MDBoxBase<MDLeanEvent<3>,3> * box;
    MDBoxIterator<MDLeanEvent<3>,3> it(top, 20, leafOnly, new SkipNothing);
    std::vector< MDBoxBase<MDLeanEvent<3>,3> * > boxes;

    // Iterate and fill the vector as you go.
    boxes.push_back(it.getBox());
    while (it.next())
    {
      box = it.getBox();
      boxes.push_back(box);
    }
    TS_ASSERT( box );
    size_t expected = 125*125*125 + 125*125 + 125 + 1;
    if (leafOnly)
      expected = 125*125*125;
    TS_ASSERT_EQUALS( boxes.size(), expected );
  }

  void test_iterator_that_fills_a_vector()
  {
    do_test_iterator_that_fills_a_vector(false);
  }

  void test_iterator_that_fills_a_vector_leafOnly()
  {
    do_test_iterator_that_fills_a_vector(true);
  }

  // ---------------------------------------------------------------
  /** For comparison, let's use getBoxes() that fills a vector directly.
   * After that, we iterate through them to compare how long the whole operation takes.
   */
  void do_test_getBoxes(bool leafOnly, int ImplicitFunction, size_t expected)
  {
      std::vector< API::IMDNode * > boxes;

    MDImplicitFunction * function = NULL;
    if (ImplicitFunction==1)
    {
      // Box in 3D where 2 < (x,y,z) < 3
      std::vector<coord_t> min(3, 2.001f);
      std::vector<coord_t> max(3, 2.999f);
      function = new MDBoxImplicitFunction(min, max);
      top->getBoxes(boxes, 20, leafOnly, function);
    }
    else if (ImplicitFunction==2)
    {
      // Plane defining 2.2 < x < 2.4
      function = new MDImplicitFunction();
      coord_t normal1[3] = {+1.000f, 0, 0};
      coord_t origin1[3] = {+2.201f, 0, 0};
      function->addPlane(MDPlane(3,normal1, origin1));
      coord_t normal2[3] = {-1.000f, 0, 0};
      coord_t origin2[3] = {+2.399f, 0, 0};
      function->addPlane(MDPlane(3,normal2, origin2));
      top->getBoxes(boxes, 20, leafOnly, function);
    }
    else if (ImplicitFunction==3)
    {
      // Box in 3D where -5 < (x,y,z) < +10
      std::vector<coord_t> min(3, -4.999f);
      std::vector<coord_t> max(3, +9.999f);
      function = new MDBoxImplicitFunction(min, max);
      top->getBoxes(boxes, 20, leafOnly, function);
    }
    else
    {
      top->getBoxes(boxes, 20, leafOnly);
    }
    TS_ASSERT_EQUALS( boxes.size(), expected);

    // Now we still need to iterate through the vector to do anything, so this is a more fair comparison
    size_t counter = 0;
    std::vector< API::IMDNode * >::iterator it;
    std::vector< API::IMDNode * >::iterator it_end = boxes.end();
    for (it = boxes.begin(); it != it_end; it++)
    {
      counter++;
    }

    TS_ASSERT_EQUALS( counter, expected );
  }

  void test_getBoxes()
  {
    do_test_getBoxes(false, 0, 125*125*125 + 125*125 + 125 + 1);
  }

  void test_getBoxes_leafOnly()
  {
    do_test_getBoxes(true, 0, 125*125*125);
  }

  void test_getBoxes_withImplicitFunction()
  {
    do_test_getBoxes(false, 1, 1 + 125*125 + 125 + 1);
  }

  void test_getBoxes_withImplicitFunction_leafOnly()
  {
    do_test_getBoxes(true, 1, 125*125);
  }

  void test_getBoxes_withPlaneImplicitFunction()
  {
    do_test_getBoxes(true, 2, 125*125*125 / 25);
  }

  void test_getBoxes_withHugeImplicitFunction()
  {
    do_test_getBoxes(true, 3, 125*125*125);   
  }

};


#endif /* MANTID_DATAOBJECTS_MDBOXITERATORTEST_H_ */
#undef RUN_CXX_PERFORMANCE_TEST_EMBEDDED
