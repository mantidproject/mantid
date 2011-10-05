#ifndef VTK_STRUCTURED_GRID_FACTORY_TEST_H_
#define VTK_STRUCTURED_GRID_FACTORY_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MockObjects.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkStructuredGridFactoryTest: public CxxTest::TestSuite
{

public:

  void testCopy()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 4);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factoryA =
      vtkStructuredGridFactory<TimeStepToTimeStep> ("signal", 0);
    factoryA.initialize(ws_sptr);

    vtkStructuredGridFactory<TimeStepToTimeStep> factoryB(factoryA);
    //Test factory copies indirectly via the products.
    vtkStructuredGrid* productA = factoryA.create();
    vtkStructuredGrid* productB = factoryB.create();

    TSM_ASSERT_EQUALS("Not copied correctly. Mesh data mismatch.", productA->GetNumberOfPoints(), productB->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Not copied correctly. Signal data mismatch.", std::string(productA->GetCellData()->GetArray(0)->GetName()), std::string(productB->GetCellData()->GetArray(0)->GetName()));
    productA->Delete();
    productB->Delete();
  }

  void testAssignment()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 4);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factoryA =
      vtkStructuredGridFactory<TimeStepToTimeStep> ("signal", 0);
    factoryA.initialize(ws_sptr);

    vtkStructuredGridFactory<TimeStepToTimeStep> factoryB =
      vtkStructuredGridFactory<TimeStepToTimeStep> ("other", 0);
    factoryB.initialize(ws_sptr);

    factoryB = factoryA;
    //Test factory assignments indirectly via the factory products.
    vtkStructuredGrid* productA = factoryA.create();
    vtkStructuredGrid* productB = factoryB.create();

    TSM_ASSERT_EQUALS("Not assigned correctly. Mesh data mismatch.", productA->GetNumberOfPoints(), productB->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Not assigned correctly. Signal data mismatch.", std::string(productA->GetCellData()->GetArray(0)->GetName()), std::string(productB->GetCellData()->GetArray(0)->GetName()));
    productA->Delete();
    productB->Delete();
  }

  void testMeshOnly()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 4);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
      vtkStructuredGridFactory<TimeStepToTimeStep>::constructAsMeshOnly();
    factory.initialize(ws_sptr);

    //Invoke mocked methods on MockIMDWorkspace.
    vtkStructuredGrid* product = factory.createMeshOnly();

    int predictedNPoints = (10 + 1) * (10 + 1) * (10 + 1);
    TSM_ASSERT_EQUALS("Wrong number of points generated", predictedNPoints, product->GetNumberOfPoints());
    product->Delete();
  }

  void testMeshOnlyCausesThrow()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 4);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
      vtkStructuredGridFactory<TimeStepToTimeStep>::constructAsMeshOnly();
    factory.initialize(ws_sptr);

    TSM_ASSERT_THROWS("Cannot access non-mesh information when factory constructed as mesh-only", factory.createScalarArray(), std::runtime_error);
  }

  void testSignalAspects()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MDHistoWorkspace_sptr ws_sptr = getFakeMDHistoWorkspace(1.0, 4);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
      vtkStructuredGridFactory<TimeStepToTimeStep> ("signal", 1);
    factory.initialize(ws_sptr);

    vtkDataSet* product = factory.create();
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    const int correctCellNumber = (10) * (10) * (10);
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkStructuredGridFactory<TimeStepToTimeStep> factory("signal", 1);

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testIsValidThrowsWhenNoTDimension()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    using namespace testing;

    IMDDimension* nullDimension = NULL;
    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    pMockWs->addDimension(nullDimension);

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);
    vtkStructuredGridFactory<TimeStepToTimeStep> factory("signal", 1);

    TSM_ASSERT_THROWS("No T dimension, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;
    vtkStructuredGridFactory<TimeStepToTimeStep> factory("signal", 1);
    TS_ASSERT_EQUALS("vtkStructuredGridFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkStructuredGridFactoryTestPerformance : public CxxTest::TestSuite
{
public:
 Mantid::API::IMDWorkspace_sptr ws_sptr;

  void setUp()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;

    // 4D, 100 bins per side
    ws_sptr = getFakeMDHistoWorkspace(1.0, 4, 100);
  }

  void testGenerateVTKDataSet()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;


    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkStructuredGridFactory<TimeStepToTimeStep> factory =
      vtkStructuredGridFactory<TimeStepToTimeStep>::constructAsMeshOnly();
    factory.initialize(ws_sptr);

    //Invoke mocked methods on MockIMDWorkspace.
    TS_ASSERT_THROWS_NOTHING(factory.createMeshOnly());
  }
};


#endif
