#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_

#include "MantidAPI/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

#include <string>
#include <vector>

class vtkDataSet;
class vtkUnstructuredGrid;

namespace Mantid
{
namespace Geometry
{
  class OrientedLattice;
}

namespace VATES
{

  /**
   * Class that handles converting a dataset from rectilinear coordinates
   * to a non-orthongonal representation.
    
    @date 11/03/2013

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
  class DLLExport vtkDataSetToNonOrthogonalDataSet 
  {
  public:
    /// Static execution method
    static void exec(vtkDataSet *dataset, std::string name);
    /// Constructor
    vtkDataSetToNonOrthogonalDataSet(vtkDataSet *dataset, std::string name);
    /// Class execution method
    void execute();
    /// Destructor
    virtual ~vtkDataSetToNonOrthogonalDataSet();
  private:
    vtkDataSetToNonOrthogonalDataSet& operator=(const vtkDataSetToNonOrthogonalDataSet& other);
    vtkDataSetToNonOrthogonalDataSet(const vtkDataSetToNonOrthogonalDataSet& other);
    /// Copy a vector to an array
    void copyToRaw(double *arr, MantidVec vec);
    /// Calculate the skew matrix and basis.
    void createSkewInformation(Geometry::OrientedLattice &ol,
                               Kernel::DblMatrix &w,
                               Kernel::Matrix<coord_t> &aff);
    /// Calculate the skew basis vector
    void findSkewBasis(Kernel::V3D &basis, double scale);
    /// Reduce the dimensionality of matrix by 1
    void stripMatrix(Kernel::DblMatrix &mat);
    /// Add the skew basis to metadata
    void updateMetaData(vtkUnstructuredGrid *ugrid);
    vtkDataSet *m_dataSet; ///< Pointer to VTK dataset to modify
    std::string m_wsName; ///< The name of the workspace to fetch
    //FIXME: Temp var for getting hardcoded stuff back
    unsigned int m_hc;
    std::size_t m_numDims; ///< Number of dimensions in workspace
    Kernel::DblMatrix m_skewMat; ///< The skew matrix for non-orthogonal representation
    MantidVec m_basisNorm; ///< Holder for the basis normalisation values
    Kernel::V3D m_basisX; ///< The X direction basis vector
    Kernel::V3D m_basisY; ///< The Y direction basis vector
    Kernel::V3D m_basisZ; ///< The Z direction basis vector
    API::SpecialCoordinateSystem m_coordType; ///< The coordinate system for the workspace
  };


} // namespace VATES
} // namespace Mantid

#endif  /* MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_ */
