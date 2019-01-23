// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKDATASET_TO_WS_NAME_H
#define VTKDATASET_TO_WS_NAME_H

#include "MantidKernel/System.h"
#include <string>

class vtkDataSet;
namespace Mantid {
namespace VATES {

/** @class vtkDataSetToImplicitFunction

Handles the extraction of existing ws location from a vtkDataSet by getting at
the field data and then processing the xml contained within.

@author Owen Arnold, Tessella Support Services plc
@date 22/08/2011
*/
class DLLExport vtkDataSetToWsName {
public:
  vtkDataSetToWsName(vtkDataSet *dataSet);
  vtkDataSetToWsName &operator=(const vtkDataSetToWsName &other) = delete;
  vtkDataSetToWsName(const vtkDataSetToWsName &other) = delete;
  static std::string exec(vtkDataSet *dataSet);
  std::string execute();
  ~vtkDataSetToWsName();

private:
  vtkDataSet *m_dataset;
};
} // namespace VATES
} // namespace Mantid

#endif