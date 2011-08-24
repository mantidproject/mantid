#ifndef BOX_FUNCTION_INTERPRETER_TEST_H_
#define BOX_FUNCTION_INTERPRETER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <boost/shared_ptr.hpp>
#include <MantidMDAlgorithms/BoxInterpreter.h>
#include <MantidMDAlgorithms/PlaneImplicitFunction.h>
#include <MantidMDAlgorithms/CompositeImplicitFunction.h>
#include <MantidMDAlgorithms/BoxImplicitFunction.h>

using namespace Mantid::MDAlgorithms;

class BoxInterpreterTest : public CxxTest::TestSuite
{


public:

   void testFindsNothing()
   {
     class FakeImplicitFunction : public Mantid::Geometry::MDImplicitFunction
     {
       virtual std::string getName() const {return "FakeImplicitFunction";}
     };


     FakeImplicitFunction fakeFunction;
     BoxInterpreter boxInterpreter;
     std::vector<double> box = boxInterpreter(&fakeFunction);
     TSM_ASSERT_EQUALS("The box min x should be zero.", 0, box[0]);
     TSM_ASSERT_EQUALS("The box max x should be zero.", 0, box[1]);
     TSM_ASSERT_EQUALS("The box min y should be zero.", 0, box[2]);
     TSM_ASSERT_EQUALS("The box max y should be zero.", 0, box[3]);
     TSM_ASSERT_EQUALS("The box min z should be zero.", 0, box[4]);
     TSM_ASSERT_EQUALS("The box max z should be zero.", 0, box[5]);
   }

    void testFindsInnerSurfaces()
    {
      OriginParameter originOne(0,0,0);
      WidthParameter widthOne(1);
      HeightParameter heightOne(4);
      DepthParameter depthOne(5);
      BoxImplicitFunction*  boxOne = new BoxImplicitFunction(widthOne, heightOne, depthOne, originOne);

      OriginParameter originTwo(0,0,0);
      WidthParameter widthTwo(2);
      HeightParameter heightTwo(3);
      DepthParameter depthTwo(6);
      BoxImplicitFunction* boxTwo = new BoxImplicitFunction(widthTwo, heightTwo, depthTwo, originTwo);

      CompositeImplicitFunction* innerComposite = new CompositeImplicitFunction;
      innerComposite->addFunction(Mantid::Geometry::MDImplicitFunction_sptr(boxTwo));

      CompositeImplicitFunction topComposite;
      topComposite.addFunction(Mantid::Geometry::MDImplicitFunction_sptr(boxOne));
      topComposite.addFunction(Mantid::Geometry::MDImplicitFunction_sptr(innerComposite));

      BoxInterpreter boxInterpreter;
      std::vector<double> box = boxInterpreter(&topComposite);

      TSM_ASSERT_EQUALS("The box min x is incorrect", -0.5, box[0]); //From box1
      TSM_ASSERT_EQUALS("The box max x is incorrect", 0.5, box[1]);  //From box1
      TSM_ASSERT_EQUALS("The box min y is incorrect", -1.5, box[2]); //From box2
      TSM_ASSERT_EQUALS("The box max y is incorrect", 1.5, box[3]);  //From box2
      TSM_ASSERT_EQUALS("The box min z is incorrect", -2.5, box[4]); //From box1
      TSM_ASSERT_EQUALS("The box max z is incorrect", 2.5, box[5]);  //From box1

    }

  void testGetAllBoxes()
  {
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

    CompositeImplicitFunction compositeFunction;
    compositeFunction.addFunction(Mantid::Geometry::MDImplicitFunction_sptr(boxOne));
    compositeFunction.addFunction(Mantid::Geometry::MDImplicitFunction_sptr(boxTwo));

    BoxInterpreter interpreter;
    boxVector boxes = interpreter.getAllBoxes(&compositeFunction);

    TSM_ASSERT_EQUALS("Wrong number of boxes returned.", 2, boxes.size());
  }

    

};



#endif
