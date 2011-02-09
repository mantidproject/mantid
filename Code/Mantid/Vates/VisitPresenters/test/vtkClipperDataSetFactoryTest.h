#ifndef VTK_CLIPPER_DATASETFACTORY_TEST_H
#define VTK_CLIPPER_DATASETFACTORY_TEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidVisitPresenters/vtkClipperDataSetFactory.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "vtkRectilinearGrid.h"

class vtkClipperDataSetFactoryTest: public CxxTest::TestSuite
{
private:

class MockClipper: public Mantid::VATES::Clipper
  {
  public:
    MOCK_METHOD1(SetInput, void(vtkDataSet* in_ds));
    MOCK_METHOD1(SetClipFunction, void(vtkImplicitFunction* func));
    MOCK_METHOD1(SetInsideOut, void(bool insideout));
    MOCK_METHOD1(SetRemoveWholeCells, void(bool removeWholeCells));
    MOCK_METHOD1(SetOutput, void(vtkUnstructuredGrid* out_ds));
    MOCK_METHOD0(Update, void());
    MOCK_METHOD0(Delete,void());
    MOCK_METHOD0(die, void());
    virtual ~MockClipper()
    {
      die();
    }
  };

 class vtkMockRectilinearGrid: public vtkRectilinearGrid
  {
  public:
    static vtkMockRectilinearGrid* New()
    {
      vtkMockRectilinearGrid* newProduct = new vtkMockRectilinearGrid();
      return newProduct;
    }
    MOCK_METHOD0(die, void());
    virtual void Delete()
    {
      die();
      vtkRectilinearGrid::Delete();
    }

  };

  // Mock type to represent other implicit functions.
  class MockImplicitFunction: public Mantid::API::ImplicitFunction
  {
  public:
    MOCK_CONST_METHOD1(evaluate, bool(const Mantid::API::Point3D* pPoint3D));
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    MOCK_METHOD0(die, void());
    ~MockImplicitFunction()
    {
      die();
    }
  };

public:

  void testCleansUp()
  {
    using Mantid::VATES::vtkClipperDataSetFactory;
    using namespace Mantid::API;

    MockImplicitFunction* mockFunction = new MockImplicitFunction;
    vtkMockRectilinearGrid* mockGrid = vtkMockRectilinearGrid::New();
    MockClipper* mockClipper = new MockClipper;
    EXPECT_CALL(*mockFunction, die()).Times(1);
    EXPECT_CALL(*mockGrid, die()).Times(0); //VisIT framework expects that input datasets are not destroyed.
    EXPECT_CALL(*mockClipper, die()).Times(1);
    {
      vtkClipperDataSetFactory factory(boost::shared_ptr<ImplicitFunction>(mockFunction), mockGrid, mockClipper);
    }

    mockGrid->Delete(); //Clean up in test scenario

    TSM_ASSERT("RAII not correct on accepted implicit function", testing::Mock::VerifyAndClearExpectations(mockFunction));
    TSM_ASSERT("RAII not correct on accepted vtkDataSet", testing::Mock::VerifyAndClearExpectations(mockGrid));
    TSM_ASSERT("RAII not correct on accepted vtkDataSet", testing::Mock::VerifyAndClearExpectations(mockClipper));
  }

  void testAppliesCuts()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;

    OriginParameter originOne(0, 0, 0);
    WidthParameter widthOne(1);
    HeightParameter heightOne(4);
    DepthParameter depthOne(5);
    BoxImplicitFunction* boxOne = new BoxImplicitFunction(widthOne, heightOne, depthOne, originOne);

    OriginParameter originTwo(0, 0, 0);
    WidthParameter widthTwo(2);
    HeightParameter heightTwo(3);
    DepthParameter depthTwo(6);
    BoxImplicitFunction* boxTwo = new BoxImplicitFunction(widthTwo, heightTwo, depthTwo, originTwo);

    CompositeImplicitFunction* compositeFunction = new CompositeImplicitFunction;
    compositeFunction->addFunction(boost::shared_ptr<ImplicitFunction>(boxOne));
    compositeFunction->addFunction(boost::shared_ptr<ImplicitFunction>(boxTwo));


    MockClipper* mockClipper = new MockClipper;
    EXPECT_CALL(*mockClipper, SetInput(testing::_)).Times(2);
    EXPECT_CALL(*mockClipper, SetClipFunction(testing::_)).Times(2);
    EXPECT_CALL(*mockClipper, SetInsideOut(true)).Times(2);
    EXPECT_CALL(*mockClipper, SetRemoveWholeCells(true)).Times(2);
    EXPECT_CALL(*mockClipper, SetOutput(testing::_)).Times(2);
    EXPECT_CALL(*mockClipper, Update()).Times(2);
    EXPECT_CALL(*mockClipper, die()).Times(1);

    {
    Mantid::VATES::vtkClipperDataSetFactory factory(boost::shared_ptr<ImplicitFunction>(compositeFunction), vtkRectilinearGrid::New(), mockClipper);
    factory.create();
    }


    TSM_ASSERT("Clipper not used correctly.", testing::Mock::VerifyAndClearExpectations(mockClipper));
  }
};
#endif
