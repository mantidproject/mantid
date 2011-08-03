#ifndef VTK_MDEW_HEXAHEDRON_FACTORY_TEST
#define VTK_MDEW_HEXAHEDRON_FACTORY_TEST

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidVatesAPI/vtkMDEWHexahedronFactory.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MockObjects.h"

#include <vtkCellData.h>
#include <vtkDataArray.h>


using namespace Mantid;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDEWHexahedronFactoryTest : public CxxTest::TestSuite
{

public:

  /* Destructive tests. Test works correctly when misused.*/

  void testGetMeshOnlyThrows()
  {
    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("Should throw. Method is not implemented.", factory.createMeshOnly(), std::runtime_error);
  }

  void testGetScalarArrayThrows()
  {
    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("Should throw. Method is not implemented.", factory.createScalarArray(), std::runtime_error);
  }

  void testCreateWithoutInitalizeThrows()
  {
    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("Have NOT initalized object. Should throw.", factory.create(), std::runtime_error);
  }

  void testInitalizeWithNullWorkspaceThrows()
  {
    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");

    IMDEventWorkspace* ws = NULL;
    TSM_ASSERT_THROWS("This is a NULL workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::runtime_error);
  }

  void testInitalizeWithWrongWorkspaceTypeThrows()
  {
    IMDWorkspace* ws = new MockIMDWorkspace;
    ws->setName("OTHER_WS_TYPE");

    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("This is an invalid workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::invalid_argument);
  }

  /*Demonstrative tests*/

  void testCreateDataSet()
  {
    vtkMDEWHexahedronFactory::MDEventWorkspace3_sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create());

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

  //TODO more tests
  //Check recurssion works properly.
  //Check threshold and NAN signal values handled propertly

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDEWHexahedronFactoryTestPerformance : public CxxTest::TestSuite
{

private:
  
  vtkMDEWHexahedronFactory::MDEventWorkspace3_sptr m_ws;

public :

  void setUp()
  {
    m_ws = MDEventsTestHelper::makeMDEW<3>(100, 0.0, 100.0, 1);
  }

  /* Create 1E6 cells*/
  void testCreateDataSet()
  {
    vtkMDEWHexahedronFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(m_ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create());

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
};


#endif
