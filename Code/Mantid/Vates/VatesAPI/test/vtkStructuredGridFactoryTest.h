#ifndef VTK_STRUCTURED_GRID_FACTORY_TEST_H_
#define VTK_STRUCTURED_GRID_FACTORY_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "vtkDataSetFactoryTest.h"
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"


class vtkStructuredGridFactoryTest: public CxxTest::TestSuite, public vtkDataSetFactoryTest
{
public:

  // Common helper method to generate a simple product mesh.
  static vtkStructuredGrid* createProduct(const int nbins)
  {
    using namespace Mantid::VATES;
    //Easy to construct image policy for testing.
    boost::shared_ptr<ImagePolicy> spImage(new ImagePolicy(nbins, nbins, nbins, nbins));
    std::string scalarName = "signal";
    const int timestep = 0;

    TimeStepToTimeStep timeMapper;
    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factory(spImage, scalarName, timestep, timeMapper);
    return factory.create();
  }

public:

//  void testCopy()
//  {
//    using namespace Mantid::VATES;
//    TimeStepToTimeStep timeMapper;
//    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factory(boost::shared_ptr<ImagePolicy>(new ImagePolicy(1, 1, 1, 1)), "", 0, timeMapper);
//    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factoryCopy(factory);
//
//    vtkStructuredGrid* productA = factory.create();
//    vtkStructuredGrid* productB = factoryCopy.create();
//
//    //Check that both products are equal.
//    TSM_ASSERT_EQUALS("Unequal number of points.", productA->GetNumberOfPoints(), productB->GetNumberOfPoints());
//    TSM_ASSERT_EQUALS("Unequal number of cell arrays.", productA->GetCellData()->GetNumberOfArrays(), productB->GetCellData()->GetNumberOfArrays());
//    productA->Delete();
//    productB->Delete();
//  }
//
//  void testAssignment()
//  {
//    using namespace Mantid::VATES;
//    TimeStepToTimeStep timeMapper;
//    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factoryA(boost::shared_ptr<ImagePolicy>(new ImagePolicy(1, 1, 1, 1)), "", 0, timeMapper );
//    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factoryB(boost::shared_ptr<ImagePolicy>(new ImagePolicy(2, 2, 2, 2)), "", 0, timeMapper );
//
//    vtkStructuredGrid* productA = factoryA.create();
//    vtkStructuredGrid* productB = factoryB.create();
//
//    //Check that both products are NOT equal.
//    TSM_ASSERT_DIFFERS("Equal number of points.", productB->GetNumberOfPoints(), productA->GetNumberOfPoints());
//
//    //Perform assignment and create products.
//    factoryA = factoryB;
//    productA = factoryA.create();
//    productB = factoryB.create();
//
//    //Check that both products are equal.
//    TSM_ASSERT_EQUALS("Unequal number of points.", productB->GetNumberOfPoints(), productA->GetNumberOfPoints());
//    productA->Delete();
//    productB->Delete();
//  }

  void testMeshOnly()
  {
    using namespace Mantid::VATES;
    TimeStepToTimeStep timeMapper;
    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factory =
    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep>::constructAsMeshOnly(boost::shared_ptr<ImagePolicy>(new ImagePolicy(1, 1, 1, 1)), timeMapper);
    vtkStructuredGrid* product = factory.createMeshOnly();
    TSM_ASSERT_EQUALS("This is not a mesh-only product.", 0,product->GetCellData()->GetNumberOfArrays());
    product->Delete();
  }

  void testMeshOnlyCausesThrow()
  {
    using namespace Mantid::VATES;
    TimeStepToTimeStep timeMapper;
    vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> factory =
        vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep>::constructAsMeshOnly(boost::shared_ptr<ImagePolicy>(
            new ImagePolicy(1, 1, 1, 1)), timeMapper);
    TSM_ASSERT_THROWS("Created a mesh-only instance and then attempted to get non-mesh information. Should throw.", factory.createScalarArray(), std::runtime_error );
  }

  void testNumberOfPointsGenerated()
  {
    using namespace Mantid::VATES;
    const int nbins = 10;
    vtkStructuredGrid* product = createProduct(nbins);
    const int correctPointNumber = (nbins + 1) * (nbins + 1) * (nbins + 1);
    TSM_ASSERT_EQUALS("The number of points in the product vtkRectilinearGrid is incorrect.", correctPointNumber, product->GetNumberOfPoints());
    product->Delete();
  }

  void testSignalDataType()
  {
    using namespace Mantid::VATES;
    const int nbins = 10;
    vtkStructuredGrid* product = createProduct(nbins);
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained signal array is not of the correct type.", std::string("vtkFloatArray"), signalData->GetClassName());
    product->Delete();
  }

  void testSignalDataName()
  {
    using namespace Mantid::VATES;
    const int nbins = 10;
    vtkStructuredGrid* product = createProduct(nbins);
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    product->Delete();
  }

  void testNumberOfArrays()
  {
    using namespace Mantid::VATES;
    const int nbins = 10;
    vtkStructuredGrid* product = createProduct(nbins);
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    product->Delete();
  }

  void testSignalDataSize()
  {
    using namespace Mantid::VATES;
    const int nbins = 10;
    vtkStructuredGrid* product = createProduct(nbins);
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    const int correctCellNumber = (nbins) * (nbins) * (nbins);
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }

  void testIsVtkDataSetFactory()
  {
    using namespace Mantid::VATES;
    TimeStepToTimeStep timeMapper;
    const vtkDataSetFactory& factory = vtkStructuredGridFactory<ImagePolicy, TimeStepToTimeStep> (boost::shared_ptr<
        ImagePolicy>(new ImagePolicy(1, 1, 1, 1)), "", 0, timeMapper);
    vtkDataSet* product = factory.create();
    TSM_ASSERT("There is no point data in the polymorhic product.", product->GetNumberOfPoints() > 0);
    product->Delete();
  }
};

#endif
