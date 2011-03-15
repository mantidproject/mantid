#ifndef VTK_RECTILINEAR_GRID_FACTORY_TEST_H_
#define VTK_RECTILINEAR_GRID_FACTORY_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "vtkDataSetFactoryTest.h"
#include "MantidVatesAPI/vtkRectilinearGridFactory.h"


using namespace Mantid::VATES;
using namespace Mantid::MDDataObjects;

class vtkRectilinearGridFactoryTest: public CxxTest::TestSuite, public vtkDataSetFactoryTest
{
private:

  // Common helper method to generate a simple product mesh.
  static vtkRectilinearGrid* createProduct(const int nbins)
  {
    //Easy to construct image policy for testing.
    boost::shared_ptr<ImagePolicy> spImage(new ImagePolicy(nbins, nbins, nbins, nbins));
    std::string scalarName = "signal";
    const int timestep = 0;

    vtkRectilinearGridFactory<ImagePolicy> factory(spImage, scalarName, timestep);
    return factory.create();
  }

  public:

  void testNumberOfPointsGenerated()
  {
    const int nbins = 10;
    vtkRectilinearGrid* product = createProduct(nbins);
    const int correctPointNumber =  (nbins + 1) * (nbins + 1) * (nbins + 1);
    TSM_ASSERT_EQUALS("The number of points in the product vtkRectilinearGrid is incorrect.", correctPointNumber, product->GetNumberOfPoints());
    product->Delete();
  }

  void testSignalDataType()
  {
    const int nbins = 10;
    vtkRectilinearGrid* product = createProduct(nbins);
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained signal array is not of the correct type.", std::string("vtkFloatArray"), signalData->GetClassName());
    product->Delete();
  }

  void testNumberOfArrays()
  {
    const int nbins = 10;
    vtkRectilinearGrid* product = createProduct(nbins);
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    product->Delete();
  }

  void testSignalDataName()
  {
    const int nbins = 10;
    vtkRectilinearGrid* product = createProduct(nbins);
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    product->Delete();
  }

  void testSignalDataSize()
  {
    const int nbins = 10;
    vtkRectilinearGrid* product = createProduct(nbins);
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    const int correctCellNumber =  (nbins) * (nbins) * (nbins);
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }


};

#endif
