#ifndef VTKNULLUNSTRUCTUREDGRID_TEST_H_
#define VTKNULLUNSTRUCTUREDGRID_TEST_H_

#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>

using namespace Mantid::VATES;

class vtkNullUnstructuredGridTest : public CxxTest::TestSuite {
public:
  void testCorrectVtkDataSetIsReturned() {
    vtkNullUnstructuredGrid grid;

    vtkUnstructuredGrid *ugrid = NULL;

    TSM_ASSERT_THROWS_NOTHING(
        "Should create the unstructured grid without problems.",
        ugrid = grid.createNullData());
    TSM_ASSERT("Should have exactly one point",
               ugrid->GetNumberOfPoints() == 1);
    TSM_ASSERT("Should have exactly one cell", ugrid->GetNumberOfCells() == 1);
    vtkPoints *p = ugrid->GetPoints();
    double coord[3];
    p->GetPoint(0, coord);
    TSM_ASSERT("X should be in the center", coord[0] == 0.0);
    TSM_ASSERT("X should be in the center", coord[1] == 0.0);
    TSM_ASSERT("X should be in the center", coord[2] == 0.0);
  }
};
#endif
