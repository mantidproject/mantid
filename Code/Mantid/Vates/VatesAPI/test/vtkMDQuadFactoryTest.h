#ifndef VTK_MD_QUAD_FACTORY_TEST
#define VTK_MD_QUAD_FACTORY_TEST 

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MockObjects.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "vtkCellType.h"
#include "vtkUnstructuredGrid.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDQuadFactoryTest : public CxxTest::TestSuite
{

public:

  void testGetFactoryTypeName()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    TS_ASSERT_EQUALS("vtkMDQuadFactory", factory.getFactoryTypeName());
  }

  void testInitializeDelegatesToSuccessor()
  {
    MockvtkDataSetFactory* mockSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.SetSuccessor(mockSuccessor);

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));

    TSM_ASSERT("Successor has not been used properly.", Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testCreateDelegatesToSuccessor()
  {
    FakeProgressAction progressUpdate;

    MockvtkDataSetFactory* mockSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, create(Ref(progressUpdate))).Times(1).WillOnce(Return(vtkStructuredGrid::New()));
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.SetSuccessor(mockSuccessor);

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));

    TSM_ASSERT("Successor has not been used properly.", Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testOnInitaliseCannotDelegateToSuccessor()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    //factory.SetSuccessor(mockSuccessor); No Successor set.

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS(factory.initialize(ws), std::runtime_error);
  }

  void testCreateWithoutInitaliseThrows()
  {
    FakeProgressAction progressUpdate;

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    //initialize not called!
    TS_ASSERT_THROWS(factory.create(progressUpdate), std::runtime_error);
  }

  void testCreation()
  {
    MockProgressAction mockProgressAction;
    //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

    boost::shared_ptr<Mantid::DataObjects::MDEventWorkspace<Mantid::DataObjects::MDEvent<2>,2> >
            ws = MDEventsTestHelper::makeMDEWFull<2>(10, 10, 10, 10);

    //Rebin it to make it possible to compare cells to bins.
    using namespace Mantid::API;
    IAlgorithm_sptr slice = AlgorithmManager::Instance().createUnmanaged("SliceMD");
    slice->initialize();
    slice->setProperty("InputWorkspace", ws);
    slice->setPropertyValue("AlignedDim0", "Axis0, -10, 10, 10");
    slice->setPropertyValue("AlignedDim1", "Axis1, -10, 10, 10");
    slice->setPropertyValue("OutputWorkspace", "binned");
    slice->execute();

    Workspace_sptr binned = Mantid::API::AnalysisDataService::Instance().retrieve("binned");

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.initialize(binned);

    vtkDataSet* product = factory.create(mockProgressAction);

    TS_ASSERT(dynamic_cast<vtkUnstructuredGrid*>(product) != NULL);
    TS_ASSERT_EQUALS(100, product->GetNumberOfCells());
    TS_ASSERT_EQUALS(400, product->GetNumberOfPoints());
    TS_ASSERT_EQUALS(VTK_QUAD, product->GetCellType(0));
    TSM_ASSERT("Progress Updates not used as expected.", Mock::VerifyAndClearExpectations(&mockProgressAction));

    product->Delete();
    AnalysisDataService::Instance().remove("binned");
  }

};

//=====================================================================================
// Peformance tests
//=====================================================================================
class vtkMDQuadFactoryTestPerformance : public CxxTest::TestSuite
{

public:

  void setUp()
  {
    boost::shared_ptr<Mantid::DataObjects::MDEventWorkspace<Mantid::DataObjects::MDEvent<2>,2> > input 
      = MDEventsTestHelper::makeMDEWFull<2>(10, 10, 10, 1000);
    //Rebin it to make it possible to compare cells to bins.
    using namespace Mantid::API;
    IAlgorithm_sptr slice = AlgorithmManager::Instance().createUnmanaged("SliceMD");
    slice->initialize();
    slice->setProperty("InputWorkspace", input);
    slice->setPropertyValue("AlignedDim0", "Axis0, -10, 10, 400");
    slice->setPropertyValue("AlignedDim1", "Axis1, -10, 10, 400");
    slice->setPropertyValue("OutputWorkspace", "binned");
    slice->execute();
  }

  void tearDown()
  {
    AnalysisDataService::Instance().remove("binned");
  }

  void testCreationOnLargeWorkspace()
  {
    FakeProgressAction progressUpdate;
    Workspace_sptr binned = Mantid::API::AnalysisDataService::Instance().retrieve("binned");

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.initialize(binned);

    vtkDataSet* product = factory.create(progressUpdate);

    TS_ASSERT(dynamic_cast<vtkUnstructuredGrid*>(product) != NULL);
    TS_ASSERT_EQUALS(160000, product->GetNumberOfCells());
    TS_ASSERT_EQUALS(640000, product->GetNumberOfPoints());

    product->Delete();
    
  }
};

#endif
  
