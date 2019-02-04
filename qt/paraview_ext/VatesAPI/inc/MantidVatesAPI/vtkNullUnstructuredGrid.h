// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATES_VTK_NULL_DATA_SET
#define VATES_VTK_NULL_DATA_SET

#include "MantidKernel/System.h"

class vtkUnstructuredGrid;

namespace Mantid {
namespace VATES {

/** Generates a vtkUnstructuredGrid with a single point. Note that this is not a
 Null
 Object for a vtkDataSet.

 @date 25/02/2015
*/

class DLLExport vtkNullUnstructuredGrid {

public:
  vtkNullUnstructuredGrid();

  ~vtkNullUnstructuredGrid();

  vtkUnstructuredGrid *createNullData();
};
} // namespace VATES
} // namespace Mantid
#endif