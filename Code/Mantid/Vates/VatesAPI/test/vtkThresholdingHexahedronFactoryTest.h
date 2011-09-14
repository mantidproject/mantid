#ifndef VTK_THRESHOLDING_HEXAHEDRON_FACTORY_TEST_H_
#define VTK_THRESHOLDING_HEXAHEDRON_FACTORY_TEST_H_

#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/vtkThresholdingHexahedronFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::MDEvents;

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkThresholdingHexahedronFactoryTest: public CxxTest::TestSuite
{

  public:

  void testThresholds()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 3);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);

    vtkThresholdingHexahedronFactory inside(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 2)), "signal");
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create());

    vtkThresholdingHexahedronFactory below(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 0.5)), "signal");
    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create());

    vtkThresholdingHexahedronFactory above(ThresholdRange_scptr(new UserDefinedThresholdRange(2, 3)), "signal");
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create());

    TS_ASSERT_EQUALS((10*10*10), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
  }

  void testSignalAspects()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 3);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingHexahedronFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    factory.initialize(ws_sptr);

    vtkDataSet* product = factory.create();
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    const int correctCellNumber = 10 * 10 * 10;
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);
    
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateMeshOnlyThrows()
  {
    using namespace Mantid::VATES;
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_THROWS(factory.createMeshOnly() , std::runtime_error);
  }

  void testCreateScalarArrayThrows()
  {
    using namespace Mantid::VATES;
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_THROWS(factory.createScalarArray() , std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    using namespace Mantid::VATES;
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).Times(1).WillOnce(Return(VecIMDDimension_const_sptr(2))); //2 dimensions on the workspace.

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);

    TSM_ASSERT("Workspace not used as expected", Mock::VerifyAndClearExpectations(pMockWs));
    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).Times(1).WillOnce(Return(VecIMDDimension_const_sptr(2))); //2 dimensions on the workspace.

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDeleagates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->setTransformFromOriginal(new NullCoordTransform);
    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).Times(2).WillRepeatedly(Return(VecIMDDimension_const_sptr(2))); //2 dimensions on the workspace.

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create()).Times(1); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingHexahedronFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

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
    vtkThresholdingHexahedronFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_EQUALS("vtkThresholdingHexahedronFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkThresholdingHexahedronFactoryTestPerformance : public CxxTest::TestSuite
{
private:

  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:

  void setUp()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing; 

    //Create the workspace. 20 bins in each dimension.
    m_ws_sptr = getFakeMDHistoWorkspace(1.0, 3, 100);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);

//    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
//    EXPECT_CALL(*pMockWs, getXDimension()).WillRepeatedly(Return(IMDDimension_const_sptr(
//        new FakeIMDDimension("x", 20))));
//    EXPECT_CALL(*pMockWs, getYDimension()).WillRepeatedly(Return(IMDDimension_const_sptr(
//        new FakeIMDDimension("y", 20))));
//    EXPECT_CALL(*pMockWs, getZDimension()).WillRepeatedly(Return(IMDDimension_const_sptr(
//        new FakeIMDDimension("z", 20))));
//    EXPECT_CALL(*pMockWs, getNonIntegratedDimensions()).WillRepeatedly(Return(VecIMDDimension_const_sptr(3)));
//    EXPECT_CALL(*pMockWs, getSignalNormalizedAt(_,_,_)).WillRepeatedly(Return(1));
//    m_ws_sptr = Mantid::API::IMDWorkspace_sptr(pMockWs);
  }

	void testGenerateHexahedronVtkDataSet()
	{
    using namespace Mantid::VATES;

    //Create the factory.
    vtkThresholdingHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    factory.initialize(m_ws_sptr);

    TS_ASSERT_THROWS_NOTHING(factory.create());
	}
};

#endif
