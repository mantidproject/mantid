#ifndef VTK_PROXY_FACTORY_TEST_H_
#define VTK_PROXY_FACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkRectilinearGrid.h>
#include "MantidVatesAPI/vtkProxyFactory.h"

class vtkProxyFactoryTest: public CxxTest::TestSuite
{
public:

  void testCreation()
  {
    using Mantid::VATES::vtkProxyFactory;

    vtkRectilinearGrid* A = vtkRectilinearGrid::New();
    vtkProxyFactory factory(A);
    vtkDataSet* B = factory.create();

    TSM_ASSERT_EQUALS("The construction parameter and product should both be of the same type", A->GetClassName(), B->GetClassName());
    TSM_ASSERT_EQUALS("The construction parameter and product should point to the same memory location", A, B);
    B->Delete();
  }

  void testCopy()
  {
    using Mantid::VATES::vtkProxyFactory;

    vtkRectilinearGrid* inputProduct = vtkRectilinearGrid::New();
    vtkProxyFactory factoryA(inputProduct);
    vtkProxyFactory copyFactory(factoryA);
    vtkDataSet* productA = factoryA.create();
    vtkDataSet* productB = copyFactory.create();

    TSM_ASSERT_EQUALS("The vtkDataSet from the original factory and copy should point to the same memory location", productA, productB);
    productA->Delete();
  }

  void testAssignment()
  {
    using Mantid::VATES::vtkProxyFactory;

    vtkRectilinearGrid* inputProductA = vtkRectilinearGrid::New();
    vtkRectilinearGrid* inputProductB = vtkRectilinearGrid::New();
    vtkProxyFactory factoryA(inputProductA);
    vtkProxyFactory factoryB(inputProductB);

    factoryA = factoryB;
    vtkDataSet* productA = factoryA.create();
    vtkDataSet* productB = factoryB.create();

    TSM_ASSERT_EQUALS("The vtkDataSet from the original factory and copy should point to the same memory location", productA, productB);
    TSM_ASSERT_EQUALS("The vtkDataSet produced by both factories should correspond to the rhs factories constructor argument", productA, inputProductB);

    inputProductA->Delete();
    inputProductB->Delete();
  }

};

#endif
