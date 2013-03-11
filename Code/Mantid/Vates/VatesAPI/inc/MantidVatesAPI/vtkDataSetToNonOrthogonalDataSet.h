#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_

#include "MantidKernel/System.h"

class vtkDataSet;
namespace Mantid
{
namespace VATES
{

  /**
   * Class that handles converting a dataset from rectilinear coordinates
   * to a non-orthongonal representation.
    
    @date 11/03/2013

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport vtkDataSetToNonOrthogonalDataSet 
  {
  public:
    /// Static execution method
    static void exec(vtkDataSet *dataset);
    /// Constructor
    vtkDataSetToNonOrthogonalDataSet(vtkDataSet *dataset);
    /// Class execution method
    void execute();
    /// Destructor
    virtual ~vtkDataSetToNonOrthogonalDataSet();
  private:
    vtkDataSetToNonOrthogonalDataSet& operator=(const vtkDataSetToNonOrthogonalDataSet& other);
    vtkDataSetToNonOrthogonalDataSet(const vtkDataSetToNonOrthogonalDataSet& other);
    vtkDataSet *m_dataSet;
  };


} // namespace VATES
} // namespace Mantid

#endif  /* MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_ */
