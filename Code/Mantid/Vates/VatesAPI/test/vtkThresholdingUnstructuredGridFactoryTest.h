#ifndef VTK_THRESHOLDING_UNSTRUCTURED_GRID_FACTORY_TEST_H_
#define VTK_THRESHOLDING_UNSTRUCTURED_GRID_FACTORY_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "vtkDataSetFactoryTest.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"


class vtkThresholdingUnstructuredGridFactoryTest: public CxxTest::TestSuite, public vtkDataSetFactoryTest
{
  // Common helper method to generate a simple product mesh.
    static vtkUnstructuredGrid* createProduct(const int nbins)
    {
      using namespace Mantid::VATES;
      //Easy to construct image policy for testing.
      boost::shared_ptr<ImagePolicy> spImage(new ImagePolicy(nbins, nbins, nbins, nbins));
      std::string scalarName = "signal";
      const int timestep = 0;

      vtkThresholdingUnstructuredGridFactory<ImagePolicy> factory(spImage, scalarName, timestep);
      return factory.create();
    }

    public:

    void testNumberOfPointsGenerated()
    {
      const int nbins = 10;
      vtkUnstructuredGrid* product = createProduct(nbins);
      const int correctPointNumber =  (nbins + 1) * (nbins + 1) * (nbins + 1);
      TSM_ASSERT_EQUALS("The number of points in the product vtkUnstructuredGrid is incorrect.", correctPointNumber, product->GetNumberOfPoints());
      product->Delete();
    }

    void testSignalDataType()
    {
      const int nbins = 10;
      vtkUnstructuredGrid* product = createProduct(nbins);
      vtkDataArray* signalData = product->GetCellData()->GetArray(0);
      TSM_ASSERT_EQUALS("The obtained signal array is not of the correct type.", std::string("vtkFloatArray"), signalData->GetClassName());
      product->Delete();
    }

    void testNumberOfArrays()
    {
      const int nbins = 10;
      vtkUnstructuredGrid* product = createProduct(nbins);
      TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
      product->Delete();
    }

    void testSignalDataName()
    {
      const int nbins = 10;
      vtkUnstructuredGrid* product = createProduct(nbins);
      vtkDataArray* signalData = product->GetCellData()->GetArray(0);
      TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
      product->Delete();
    }

    void testSignalDataSize()
    {
      const int nbins = 10;
      vtkUnstructuredGrid* product = createProduct(nbins);
      vtkDataArray* signalData = product->GetCellData()->GetArray(0);
      const int correctCellNumber =  (nbins - 1) * (nbins) * (nbins); //see getPoints implemenation in base class. when i is zero signal is zero.
      TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
      product->Delete();
    }

    void testIsVtkDataSetFactory()
    {
      using namespace Mantid::VATES;
       const vtkDataSetFactory& factory = vtkThresholdingUnstructuredGridFactory<ImagePolicy>(boost::shared_ptr<ImagePolicy>(new ImagePolicy(1, 1, 1, 1)), "", 0);
       vtkDataSet* product = factory.create();
       TSM_ASSERT("There is no point data in the polymorhic product.", product->GetNumberOfPoints() > 0);
       product->Delete();
    }



};

#endif
