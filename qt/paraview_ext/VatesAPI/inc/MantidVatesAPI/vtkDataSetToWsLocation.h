// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKDATASET_TO_WS_LOCATION_H
#define VTKDATASET_TO_WS_LOCATION_H

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
class DLLExport vtkDataSetToWsLocation {
public:
  vtkDataSetToWsLocation &
  operator=(const vtkDataSetToWsLocation &other) = delete;
  vtkDataSetToWsLocation(const vtkDataSetToWsLocation &other) = delete;
  static std::string exec(vtkDataSet *dataSet);
  vtkDataSetToWsLocation(vtkDataSet *dataSet);
  std::string execute();
  ~vtkDataSetToWsLocation();

private:
  vtkDataSet *m_dataset;
};
} // namespace VATES
} // namespace Mantid

#endif