#ifndef VTK_MD_HEX_FACTORY_TEST
#define VTK_MD_HEX_FACTORY_TEST

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDHexFactoryTest : public CxxTest::TestSuite
{
private:

  void doDimensionalityTesting(bool doCheckDimensionality)
  {
    Mantid::DataObjects::MDEventWorkspace3Lean::sptr input_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);

    using namespace Mantid::API;
    IAlgorithm_sptr slice = AlgorithmManager::Instance().createUnmanaged("SliceMD");
    slice->initialize();
    slice->setProperty("InputWorkspace", input_ws);
    slice->setPropertyValue("AlignedDim0", "Axis0, -10, 10, 1");
    slice->setPropertyValue("AlignedDim0", "Axis1, -10, 10, 1");
    slice->setPropertyValue("AlignedDim0", "Axis2, -10, 10, 1");
    slice->setPropertyValue("OutputWorkspace", "binned");
    slice->execute();

    Workspace_sptr binned_ws = AnalysisDataService::Instance().retrieve("binned");
    FakeProgressAction progressUpdater;

    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.setCheckDimensionality(doCheckDimensionality);
    if(doCheckDimensionality)
    {
      TS_ASSERT_THROWS(factory.initialize(binned_ws), std::runtime_error);
    }
    else
    {
      TS_ASSERT_THROWS_NOTHING(factory.initialize(binned_ws));
      vtkDataSet* product = NULL;
      TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdater));
      product->Delete();
    }
  }

public:

  /* Destructive tests. Test works correctly when misused.*/

  void testCreateWithoutInitalizeThrows()
  {
    FakeProgressAction progressUpdater;
    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("Have NOT initalized object. Should throw.", factory.create(progressUpdater), std::runtime_error);
  }

  void testInitalizeWithNullWorkspaceThrows()
  {
    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");

    IMDEventWorkspace* ws = NULL;
    TSM_ASSERT_THROWS("This is a NULL workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::invalid_argument);
  }


  void testGetFactoryTypeName()
  {
    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    TS_ASSERT_EQUALS("vtkMDHexFactory", factory.getFactoryTypeName());
  }

  void testInitializeDelegatesToSuccessor()
  {
    MockvtkDataSetFactory* mockSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.SetSuccessor(mockSuccessor);

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));

    TSM_ASSERT("Successor has not been used properly.", Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testCreateDelegatesToSuccessor()
  {
    FakeProgressAction progressUpdater;
    MockvtkDataSetFactory* mockSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, create(Ref(progressUpdater))).Times(1).WillOnce(Return(vtkStructuredGrid::New()));
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.SetSuccessor(mockSuccessor);

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdater));

    TSM_ASSERT("Successor has not been used properly.", Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testOnInitaliseCannotDelegateToSuccessor()
  {
    FakeProgressAction progressUpdater;
    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    //factory.SetSuccessor(mockSuccessor); No Successor set.

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS(factory.initialize(ws), std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressUpdater;
    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    //initialize not called!
    TS_ASSERT_THROWS(factory.create(progressUpdater), std::runtime_error);
  }

  /*Demonstrative tests*/
  void testIgnoresDimensionality()
  {
    doDimensionalityTesting(true);
  }

  void testDoNotIgnoreDimensionality()
  {
    doDimensionalityTesting(false);
  }

  void test_3DWorkspace()
  {
    FakeProgressAction progressUpdate;

    Mantid::DataObjects::MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    const size_t expected_n_points = 8000;
    const size_t expected_n_cells = 1000;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("Wrong number of points to cells. Hexahedron has 8 vertexes.", expected_n_cells * 8,  product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());
    
    /*Check dataset bounds*/
    double* bounds = product->GetBounds();
    TS_ASSERT_EQUALS(0, bounds[0]);
    TS_ASSERT_EQUALS(10, bounds[1]);
    TS_ASSERT_EQUALS(0, bounds[2]);
    TS_ASSERT_EQUALS(10, bounds[3]);
    TS_ASSERT_EQUALS(0, bounds[4]);
    TS_ASSERT_EQUALS(10, bounds[5]);

    product->Delete();
  }

  void test_4DWorkspace()
  {
    MockProgressAction mockProgressAction;
    EXPECT_CALL(mockProgressAction, eventRaised(_)).Times(AtLeast(1));

    Mantid::DataObjects::MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(5, -10.0, 10.0, 1);
    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(mockProgressAction));

    const size_t expected_n_points = 8*125;
    const size_t expected_n_cells = 125;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("Wrong number of points to cells. Hexahedron has 8 vertexes.", expected_n_cells * 8,  product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());

    /*Check dataset bounds*/
    double* bounds = product->GetBounds();
    TS_ASSERT_EQUALS(-10.0, bounds[0]);
    TS_ASSERT_EQUALS(10, bounds[1]);
    TS_ASSERT_EQUALS(-10, bounds[2]);
    TS_ASSERT_EQUALS(10, bounds[3]);
    TS_ASSERT_EQUALS(-10, bounds[4]);
    TS_ASSERT_EQUALS(10, bounds[5]);

    TSM_ASSERT("Progress reporting has not been conducted as expected", Mock::VerifyAndClearExpectations(&mockProgressAction));

    product->Delete();
  }


};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDHexFactoryTestPerformance : public CxxTest::TestSuite
{

private:
  
  Mantid::DataObjects::MDEventWorkspace3Lean::sptr m_ws3;
  Mantid::DataObjects::MDEventWorkspace4Lean::sptr m_ws4;

public :

  void setUp()
  {
    m_ws3 = MDEventsTestHelper::makeMDEW<3>(100, 0.0, 100.0, 1);
    m_ws4 = MDEventsTestHelper::makeMDEW<4>(32, -50.0, 50.0, 1);
  }

  /* Create 1E6 cells*/
  void test_CreateDataSet_from3D()
  {
    FakeProgressAction progressUpdate;

    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(m_ws3);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    const size_t expected_n_points = 8000000;
    const size_t expected_n_cells = 1000000;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("Wrong number of points to cells. Hexahedron has 8 vertexes.", expected_n_cells * 8,  product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());
    
    if (false)
    {
      /*Check dataset bounds - this call takes a significant amount of time and so should only be used for debugging.*/
      double* bounds = product->GetBounds();
      TS_ASSERT_EQUALS(0, bounds[0]);
      TS_ASSERT_EQUALS(100, bounds[1]);
      TS_ASSERT_EQUALS(0, bounds[2]);
      TS_ASSERT_EQUALS(100, bounds[3]);
      TS_ASSERT_EQUALS(0, bounds[4]);
      TS_ASSERT_EQUALS(100, bounds[5]);
    }


    product->Delete();
  }
  /* Create 1E6 cells*/
  void test_CreateDataSet_from4D()
  {
    FakeProgressAction progressUpdate;

    vtkMDHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(m_ws4);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    const size_t expected_n_points = 8*65536;
    const size_t expected_n_cells = 65536;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("Wrong number of points to cells. Hexahedron has 8 vertexes.", expected_n_cells * 8,  product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());

    product->Delete();
  }
};


#endif
