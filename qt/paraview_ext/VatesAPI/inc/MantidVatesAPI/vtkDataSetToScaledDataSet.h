// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTKDATASETTOSCALEDDATASET_H_
#define MANTID_VATES_VTKDATASETTOSCALEDDATASET_H_

#include "MantidKernel/System.h"

class vtkPointSet;
class vtkInformation;
namespace Mantid {
namespace VATES {

/**
 *Functor class that handles scaling a given vtkDataSet and setting appropriate
 *metadata on output vtkDataSet so that original extents will be shown.

  @date 22/02/2013
*/
class DLLExport vtkDataSetToScaledDataSet {
public:
  /// Constructor
  vtkDataSetToScaledDataSet();
  vtkDataSetToScaledDataSet(const vtkDataSetToScaledDataSet &) = delete;
  vtkDataSetToScaledDataSet &
  operator=(const vtkDataSetToScaledDataSet &) = delete;
  /// Destructor
  virtual ~vtkDataSetToScaledDataSet();
  /// Apply the scaling and add metadata
  vtkPointSet *execute(double xScale, double yScale, double zScale,
                       vtkPointSet *inputData, vtkInformation *info);
  /// Apply the scaling and add metadata
  vtkPointSet *execute(double xScale, double yScale, double zScale,
                       vtkPointSet *inputData,
                       vtkPointSet *outputData = nullptr);

private:
  /// Set metadata on the dataset to handle scaling
  void updateMetaData(double xScale, double yScale, double zScale,
                      vtkPointSet *inputData, vtkPointSet *outputData);
};

} // namespace VATES
} // namespace Mantid

#endif // MANTID_VATES_VTKDATASETTOSCALEDDATASET_H_
