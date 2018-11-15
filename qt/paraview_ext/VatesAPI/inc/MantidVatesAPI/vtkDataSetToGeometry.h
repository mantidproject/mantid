// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKDATASET_TO_GEOMETRY_H_
#define VTKDATASET_TO_GEOMETRY_H_

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidKernel/System.h"

class vtkDataSet;
namespace Mantid {
namespace VATES {

/** @class vtkDataSetToGeometry

Handles the extraction of dimensions from a vtkDataSet by getting at the field
data and then processing the xml contained within to determine how mappings have
been formed.

@author Owen Arnold, Tessella Support Services plc
@date 13/05/2011
*/
class DLLExport vtkDataSetToGeometry
    : public Mantid::Geometry::MDGeometryXMLParser {

private:
  vtkDataSet *m_dataSet;

public:
  explicit vtkDataSetToGeometry(vtkDataSet *dataSet);

  ~vtkDataSetToGeometry() override;

  void execute() override;

  vtkDataSetToGeometry(const vtkDataSetToGeometry &other);

  vtkDataSetToGeometry &operator=(const vtkDataSetToGeometry &other);
};
} // namespace VATES
} // namespace Mantid

#endif