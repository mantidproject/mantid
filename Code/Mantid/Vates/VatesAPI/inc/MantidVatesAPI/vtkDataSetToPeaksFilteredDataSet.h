#ifndef MANTID_VATES_PeaksFilter_H
#define MANTID_VATES_PeaksFilter_H

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <string>

class vtkUnstructuredGrid;

namespace Mantid
{
namespace VATES
{

  /**
    Class that selects only those data points which lie within the geometry of a peak. 
    
    @date 17/02/2015

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  class DLLExport vtkDataSetToPeaksFilteredDataSet
  {
    public:
      vtkDataSetToPeaksFilteredDataSet(vtkUnstructuredGrid *input, vtkUnstructuredGrid *output);
      virtual ~vtkDataSetToPeaksFilteredDataSet();
      /// Set the name of the peaks workspace
      void initialize(std::string peaksWorkpspaceName, double radiusNoShape, int radiusType);
      /// Apply the peak filtering
      void execute();
    private:
      vtkDataSetToPeaksFilteredDataSet& operator=(const vtkDataSetToPeaksFilteredDataSet& other);
      std::vector<std::pair<Mantid::Kernel::V3D, double>> getPeaksInfo();
      vtkUnstructuredGrid *m_inputData; ///< Data to peak filter
      vtkUnstructuredGrid *m_outputData; ///< Peak filtered data
      std::string m_peaksWorkspaceName; ///< The name of the peaks workspace.
      bool m_isInitialised; ///<Flag if the filter is initialized
      double m_radiusNoShape; ///< The radius for peaks with no peak shape.
      int m_radiusType;
  };
}
}
#endif