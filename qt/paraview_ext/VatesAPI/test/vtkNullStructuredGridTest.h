#ifndef VTKNULLSTRUCTUREDGRID_TEST_H_
#define VTKNULLSTRUCTUREDGRID_TEST_H_

#include "MantidVatesAPI/vtkNullStructuredGrid.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>

using namespace Mantid::VATES;

class vtkNullStructuredGridTest : public CxxTest::TestSuite {
public:
  void testCorrectVtkDataSetIsReturned() {
    vtkNullStructuredGrid grid;

    vtkSmartPointer<vtkStructuredGrid> ugrid;

    TSM_ASSERT_THROWS_NOTHING(
        "Should create the unstructured grid without problems.",
        ugrid.TakeReference(grid.createNullData()));
    TSM_ASSERT("Should have exactly one point",
               ugrid->GetNumberOfPoints() == 1);
    TSM_ASSERT("Should have exactly one cell", ugrid->GetNumberOfCells() == 1);
    double coord[3];
    ugrid->GetPoint(0, coord);
    TSM_ASSERT("X should be in the center", coord[0] == 0.0);
    TSM_ASSERT("X should be in the center", coord[1] == 0.0);
    TSM_ASSERT("X should be in the center", coord[2] == 0.0);
  }
};
#endif
