#ifndef VTK_THRESHOLDING_QUAD_FACTORY_TEST_H
#define VTK_THRESHOLDING_QUAD_FACTORY_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "MantidAPI/IMDIterator.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include "MockObjects.h"

using namespace Mantid;
using namespace Mantid::VATES;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkThresholdingQuadFactoryTest: public CxxTest::TestSuite
{

public:
  void testCreateMeshOnlyThrows()
  {
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_THROWS(factory.createMeshOnly() , std::runtime_error);
  }

  void testCreateScalarArrayThrows()
  {
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_THROWS(factory.createScalarArray() , std::runtime_error);
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testInsideThresholds()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->setTransformFromOriginal(new NullCoordTransform);
    pMockWs->addDimension(new FakeIMDDimension("x"));
    pMockWs->addDimension(new FakeIMDDimension("y"));
    EXPECT_CALL(*pMockWs, getSignalNormalizedAt(_,_)).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillRepeatedly(Return(VecIMDDimension_const_sptr(2)));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 2);
    vtkThresholdingQuadFactory inside(ThresholdRange_scptr(pRange), "signal");
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create());

    TS_ASSERT_EQUALS((10*10), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS((11*11), insideProduct->GetNumberOfPoints());
  }

  void testAboveThreshold()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->setTransformFromOriginal(new NullCoordTransform);
    pMockWs->addDimension(new FakeIMDDimension("x"));
    pMockWs->addDimension(new FakeIMDDimension("y"));
    EXPECT_CALL(*pMockWs, getSignalNormalizedAt(_,_)).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillRepeatedly(Return(VecIMDDimension_const_sptr(2)));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall above and outside the minimum 0 and maximum 0.5.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 0.5);
    vtkThresholdingQuadFactory above(ThresholdRange_scptr(pRange), "signal");
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create());

    // No points nor cells are created if nothing is within range
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfPoints());
  }

  void testBelowThreshold()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->setTransformFromOriginal(new NullCoordTransform);
    pMockWs->addDimension(new FakeIMDDimension("x"));
    pMockWs->addDimension(new FakeIMDDimension("y"));
    EXPECT_CALL(*pMockWs, getSignalNormalizedAt(_,_)).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillRepeatedly(Return(VecIMDDimension_const_sptr(2)));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall below and outside the minimum 1.5 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(1.5, 2);
    vtkThresholdingQuadFactory below(ThresholdRange_scptr(pRange), "signal");

    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create());

    // No points nor cells are created if nothing is within range
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfPoints());
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillOnce(Return(VecIMDDimension_const_sptr(1)));; //1 dimensions on the workspace.

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);

    TSM_ASSERT("Workspace not used as expected", Mock::VerifyAndClearExpectations(pMockWs));
    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillOnce(Return(VecIMDDimension_const_sptr(1))); //1 dimensions on the workspace.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDeleagates()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->setTransformFromOriginal(new NullCoordTransform);
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).Times(2).WillRepeatedly(Return(VecIMDDimension_const_sptr(1))); //1 dimensions on the workspace.

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create()).Times(1); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);
    factory.create(); // should be called on successor.

    TSM_ASSERT("Workspace not used as expected", Mock::VerifyAndClearExpectations(pMockWs));
    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  
  void testTypeName()
  {
    using namespace Mantid::VATES;
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_EQUALS("vtkThresholdingQuadFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkThresholdingQuadFactoryTestPerformance : public CxxTest::TestSuite
{
private:
  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:

  void setUp()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->addDimension(new FakeIMDDimension("x", 100));
    pMockWs->addDimension(new FakeIMDDimension("y", 100));
    EXPECT_CALL(*pMockWs, getSignalNormalizedAt(_,_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillRepeatedly(Return(VecIMDDimension_const_sptr(2)));

    m_ws_sptr = Mantid::API::IMDWorkspace_sptr(pMockWs);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);
  }

	void testGenerateVTKDataSet()
	{
    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create());
	}
};

#endif
