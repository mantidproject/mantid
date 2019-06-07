// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MATRIXPROPERTYTEST_H_
#define MATRIXPROPERTYTEST_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/MatrixProperty.h"
#include "MantidKernel/PropertyManager.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::IntMatrix;
using Mantid::Kernel::MatrixProperty;
using Mantid::Kernel::PropertyManager;

class MatrixPropertyTest : public CxxTest::TestSuite {
public:
  void test_That_Default_Contruction_Gives_Empty_Matrix() {
    MatrixProperty<double> prop("Rot");
    TS_ASSERT_EQUALS(int(prop.direction()),
                     int(Mantid::Kernel::Direction::Input));
    DblMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 0);
    TS_ASSERT_EQUALS(R.numRows(), 0);
  }

  void
  test_That_After_SetValue_With_Valid_String_The_Same_Matrix_Values_Are_Returned() {
    MatrixProperty<double> prop("Rot");
    std::string error = prop.setValue("Matrix(3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_EQUALS(error, "");
    DblMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 3);
    TS_ASSERT_EQUALS(R.numRows(), 3);
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        TS_ASSERT_EQUALS(R[i][j], static_cast<double>(i * R.numRows() + j + 1));
      }
    }
  }

  void test_That_SetValue_With_Invalid_Input_Returns_An_Error_Message() {
    MatrixProperty<double> prop("Rot");
    std::string error = prop.setValue("1,2,3,4,5,6,7,8,9");
    TS_ASSERT_EQUALS(error, "Incorrect input format for Matrix stream.");
    error = prop.setValue("1");
    TS_ASSERT_EQUALS(error,
                     "Unexpected character when reading Matrix from stream.");
    // Left at default
    DblMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 0);
    TS_ASSERT_EQUALS(R.numRows(), 0);
  }

  void test_Valid_Input_With_Integers() {
    MatrixProperty<int> prop("Identity");
    std::string error = prop.setValue("Matrix(3,3)1,0,0,0,1,0,0,0,1");
    TS_ASSERT_EQUALS(error, "");
    IntMatrix R = prop();
    TS_ASSERT_EQUALS(R.numCols(), 3);
    TS_ASSERT_EQUALS(R.numRows(), 3);
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        if (i == j) {
          TS_ASSERT_EQUALS(R[i][j], 1);
        } else {
          TS_ASSERT_EQUALS(R[i][j], 0);
        }
      }
    }
  }

  //      declareProperty(new MatrixProperty<>("Rotation"));// Defaults to
  //      double type

  void test_Extracting_From_PropertyManager_Succeeds() {
    boost::shared_ptr<PropertyManager> manager =
        boost::make_shared<PropertyManager>();
    manager->declareProperty(std::make_unique<MatrixProperty<>>("Rotation"),
                             "Rotation matrix"); // Default is null
    DblMatrix null = manager->getProperty("Rotation");
    TS_ASSERT_EQUALS(null.numRows(), 0);
    TS_ASSERT_EQUALS(null.numCols(), 0);
    // Set something else and test it comes back okay
    DblMatrix identity(3, 3, true); // Identity
    TS_ASSERT_THROWS_NOTHING(manager->setProperty("Rotation", identity));
    TS_ASSERT_EQUALS(identity.numRows(), 3);
    TS_ASSERT_EQUALS(identity.numCols(), 3);
    if (identity.numCols() == 3) {
      for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
          if (i == j) {
            TS_ASSERT_EQUALS(identity[i][j], 1.0);
          } else {
            TS_ASSERT_EQUALS(identity[i][j], 0.0);
          }
        }
      }
    }
  }
};

#endif
