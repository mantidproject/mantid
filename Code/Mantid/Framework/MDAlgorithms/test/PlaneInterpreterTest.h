#ifndef PLANE_FUNCTION_INTERPRETER_TEST_H_
#define PLANE_FUNCTION_INTERPRETER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <boost/shared_ptr.hpp>
#include <MantidGeometry/Math/Matrix.h>
#include <MantidMDAlgorithms/NormalParameter.h>
#include <MantidMDAlgorithms/OriginParameter.h>
#include <MantidMDAlgorithms/PlaneInterpreter.h>
#include <MantidMDAlgorithms/PlaneImplicitFunction.h>
#include <MantidMDAlgorithms/CompositeImplicitFunction.h>
#include <gsl/gsl_blas.h>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;
using namespace Mantid::Geometry;

class PlaneInterpreterTest: public CxxTest::TestSuite
{

private:

  // Mock type to represent other implicit functions.
  class MockImplicitFunction : public Mantid::API::ImplicitFunction
  {
   public:
    MOCK_CONST_METHOD1(evaluate, bool(const Mantid::API::Point3D* pPoint3D));
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunction()
    {
    }
  };

  // Helper method to determine wheter a given vector represents an identity matrix.
  bool isIdentityMatrix(std::vector<double> rotationMatrix)
  {
    double identityArry[] ={1, 0, 0, 0, 1, 0, 0, 0, 1};
    std::vector<double> identityVector(identityArry, identityArry + sizeof(identityArry)/sizeof(*identityArry));

    return rotationMatrix == identityVector;
  }

public:

  void testNoCompositeGivesDefault()
  {
    MockImplicitFunction mockFunction;
    EXPECT_CALL(mockFunction, getName()).Times(0); // Should never get this far. as no composite available.

    PlaneInterpreter interpreter;
    std::vector<double> rotationMatrix = interpreter(&mockFunction);
    TSM_ASSERT_EQUALS("An identity matrix was expected.", true, isIdentityMatrix(rotationMatrix));
  }

  void testNoPlanesGivesDefault()
  {
    CompositeImplicitFunction compositeFunction;
    MockImplicitFunction* mockFunction = new MockImplicitFunction;
    EXPECT_CALL(*mockFunction, getName()).Times(2).WillRepeatedly(testing::Return("MockFunction"));

    boost::shared_ptr<MockImplicitFunction> spMockFunction(mockFunction);
    compositeFunction.addFunction(spMockFunction);

    PlaneInterpreter interpreter;
    std::vector<double> rotationMatrix = interpreter(&compositeFunction);
    TSM_ASSERT_EQUALS("An identity matrix was expected.", true, isIdentityMatrix(rotationMatrix));
  }

  void testLastPlaneApplied()
  {
    CompositeImplicitFunction compositeFunction;
    NormalParameter normalA(1, 0, 0);
    NormalParameter normalB(1, 1, 1);
    OriginParameter origin(0, 0, 0);
    boost::shared_ptr<PlaneImplicitFunction> functionA(new PlaneImplicitFunction(normalA, origin));
    boost::shared_ptr<PlaneImplicitFunction> functionB(new PlaneImplicitFunction(normalB, origin));
    compositeFunction.addFunction(functionA);
    compositeFunction.addFunction(functionB);
    PlaneInterpreter interpreter;
    std::vector<double> rotationMatrix = interpreter(&compositeFunction);

    TSM_ASSERT_EQUALS("Should not be a default identity matrix.", true, !isIdentityMatrix(rotationMatrix));
    TSM_ASSERT_EQUALS("The end rotation matrix should correspond to that of the last plane.", functionB->asRotationMatrixVector() , rotationMatrix);
  }

};



#endif
