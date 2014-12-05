#ifndef MANTID_VATES_VTKDATASETTOSCALEDDATASET_H_
#define MANTID_VATES_VTKDATASETTOSCALEDDATASET_H_

#include "MantidKernel/System.h"

class vtkUnstructuredGrid;
namespace Mantid
{
namespace VATES
{

  /**
   *Class that handles scaling a given vtkDataSet and setting appropriate
   *metadata on output vtkDataSet so that original extents will be shown.
    
    @date 22/02/2013

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport vtkDataSetToScaledDataSet 
  {
  public:
    /// Constructor
    vtkDataSetToScaledDataSet(vtkUnstructuredGrid *input,
                              vtkUnstructuredGrid *output);
    /// Destructor
    virtual ~vtkDataSetToScaledDataSet();
    /// Set the scaling factors
    void initialize(double xScale, double yScale, double zScale);
    /// Apply the scaling and add metadata
    void execute();
  private:
    vtkDataSetToScaledDataSet& operator=(const vtkDataSetToScaledDataSet& other);
    /// Set metadata on the dataset to handle scaling
    void updateMetaData();

    vtkUnstructuredGrid *m_inputData; ///< Data to scale
    vtkUnstructuredGrid *m_outputData; ///< Scaled data
    double m_xScaling; ///< The scale factor in the X direction
    double m_yScaling; ///< The scale factor in the Y direction
    double m_zScaling; ///< The scale factor in the Z direction
    bool m_isInitialised; ///< Flag to declare object initialised
  };

} // namespace VATES
} // namespace Mantid

#endif // MANTID_VATES_VTKDATASETTOSCALEDDATASET_H_
