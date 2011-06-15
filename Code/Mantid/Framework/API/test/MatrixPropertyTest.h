#ifndef MATRIXPROPERTYTEST_H_
#define MATRIXPROPERTYTEST_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/MatrixProperty.h"
#include "MantidAPI/Algorithm.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::MatrixProperty;
using Mantid::Geometry::DblMatrix;
using Mantid::Geometry::IntMatrix;

class MatrixPropertyTest : public CxxTest::TestSuite
{
public:

  void test_That_Default_Contruction_Gives_Empty_Matrix()
  {
    MatrixProperty<double> prop("Rot");
    TS_ASSERT_EQUALS(prop.direction(), Mantid::Kernel::Direction::Input);
    DblMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 0);
    TS_ASSERT_EQUALS(R.numRows(), 0);
  }

  void test_That_After_SetValue_With_Valid_String_The_Same_Matrix_Values_Are_Returned()
  {
    MatrixProperty<double> prop("Rot");
    std::string error = prop.setValue("Matrix(3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_EQUALS(error, "");
    DblMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 3);
    TS_ASSERT_EQUALS(R.numRows(), 3);
    for( size_t i = 0; i < 3; ++i )
    {
      for( size_t j = 0; j < 3; ++j )
      {
        TS_ASSERT_EQUALS(R[i][j], static_cast<double>(i*R.numRows() + j + 1));
      }
    }
  }

  void test_That_SetValue_With_Invalid_Input_Returns_An_Error_Message()
  {
    MatrixProperty<double> prop("Rot");
    std::string error = prop.setValue("1,2,3,4,5,6,7,8,9");
    TS_ASSERT_EQUALS(error, "Incorrect input format for Matrix stream.");
    error = prop.setValue("1");
    TS_ASSERT_EQUALS(error, "Unexpected character when reading Matrix from stream.");
    // Left at default
    DblMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 0);
    TS_ASSERT_EQUALS(R.numRows(), 0);
  }

  void test_Valid_Input_With_Integers()
  {
    MatrixProperty<int> prop("Identity");
    std::string error = prop.setValue("Matrix(3,3)1,0,0,0,1,0,0,0,1");
    TS_ASSERT_EQUALS(error, "");
    IntMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 3);
    TS_ASSERT_EQUALS(R.numRows(), 3);
    for( size_t i = 0; i < 3; ++i )
    {
      for( size_t j = 0; j < 3; ++j )
      {
        if( i == j ) 
        {
          TS_ASSERT_EQUALS(R[i][j], 1);
        }
        else
        {
          TS_ASSERT_EQUALS(R[i][j], 0);
        }
      }
    }
  }

  class AlgorithmWithMatrixProperty : public Mantid::API::Algorithm
  {
  public:
    const std::string name() const { return "AlgorithmWithMatrixProperty";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat";} 

    void init()
    { 
      declareProperty(new MatrixProperty<>("Rotation"));// Defaults to double type
    }
    void exec() 
    {
      // Cannot figure out a way to do the direct assignment while keeping
      // Visual Studio happy
      Mantid::Kernel::Property * base = getProperty("Rotation");
      MatrixProperty<> * p = dynamic_cast<MatrixProperty<> *>(base);
      if( p )
      {
        DblMatrix R = (*p)(); 
      }
      else
      {
        throw std::runtime_error("Cannot retrieve MatrixProperty");
      }
    }

  };

    void test_Input_To_Algorithm()
    {
      boost::shared_ptr<Mantid::API::IAlgorithm> testAlg(new AlgorithmWithMatrixProperty);
      TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
      TS_ASSERT_THROWS_NOTHING(testAlg->execute()); 
      TS_ASSERT(testAlg->isExecuted()); //Default should be okay

      //Is that what we get back ?
      Mantid::Kernel::Property * base = testAlg->getProperty("Rotation");
      MatrixProperty<> * p = dynamic_cast<MatrixProperty<> *>(base);
      DblMatrix propValue = (*p)();
      TS_ASSERT_EQUALS(propValue.numRows(), 0);
      TS_ASSERT_EQUALS(propValue.numCols(), 0);

      DblMatrix ubMatrix(3,3,true); //Identity
      testAlg->setProperty("Rotation", ubMatrix);
      TS_ASSERT_THROWS_NOTHING(testAlg->execute()); 
      TS_ASSERT(testAlg->isExecuted()); 
      base = testAlg->getProperty("Rotation");
      p = dynamic_cast<MatrixProperty<> *>(base);
      propValue = (*p)();
      TS_ASSERT_EQUALS(propValue.numRows(), 3);
      TS_ASSERT_EQUALS(propValue.numCols(), 3);
      if( propValue.numCols() == 3 )
      {
        for( size_t i = 0; i < 3; ++i )
        {
          for( size_t j = 0; j < 3; ++j )
          {
            if( i == j )
            {
              TS_ASSERT_EQUALS(propValue[i][j], 1.0);
            }
            else
            {
              TS_ASSERT_EQUALS(propValue[i][j], 0.0);
            }
          }
        }
      }

  }


};

#endif