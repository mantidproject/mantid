// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKDATASET_TO_IMPLICIT_FUNCTION_H_
#define VTKDATASET_TO_IMPLICIT_FUNCTION_H_

#include "MantidKernel/System.h"

class vtkDataSet;
namespace Mantid {
namespace Geometry {
// Forward declaration.
class MDImplicitFunction;
} // namespace Geometry
namespace VATES {

/** @class vtkDataSetToImplicitFunction

Handles the extraction of existing implcitfunctions from a vtkDataSet by getting
at the field data and then processing the xml contained within.

@author Owen Arnold, Tessella Support Services plc
@date 22/08/2011
*/
class DLLExport vtkDataSetToImplicitFunction {
public:
  vtkDataSetToImplicitFunction(vtkDataSet *dataSet);
  vtkDataSetToImplicitFunction &
  operator=(const vtkDataSetToImplicitFunction &other) = delete;
  vtkDataSetToImplicitFunction(const vtkDataSetToImplicitFunction &other) =
      delete;
  static Mantid::Geometry::MDImplicitFunction *exec(vtkDataSet *dataSet);
  Mantid::Geometry::MDImplicitFunction *execute();
  ~vtkDataSetToImplicitFunction();

private:
  vtkDataSet *m_dataset;
};
} // namespace VATES
} // namespace Mantid

#endif
