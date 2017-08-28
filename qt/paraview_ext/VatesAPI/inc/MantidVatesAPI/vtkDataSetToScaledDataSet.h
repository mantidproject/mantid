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

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
