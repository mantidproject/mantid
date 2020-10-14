// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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