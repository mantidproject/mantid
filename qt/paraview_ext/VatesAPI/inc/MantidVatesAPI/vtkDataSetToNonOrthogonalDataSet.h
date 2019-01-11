// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/WorkspaceProvider.h"

#include <array>
#include <string>

class vtkDataSet;

namespace Mantid {
namespace Geometry {
class OrientedLattice;
}

namespace VATES {

/**
 * Class that handles converting a dataset from rectilinear coordinates
 * to a non-orthongonal representation.

  @date 11/03/2013
*/
class DLLExport vtkDataSetToNonOrthogonalDataSet {
public:
  /// Static execution method
  static void exec(vtkDataSet *dataset, std::string name,
                   std::unique_ptr<WorkspaceProvider> workspaceProvider);
  /// Constructor
  vtkDataSetToNonOrthogonalDataSet(
      vtkDataSet *dataset, std::string name,
      std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider);
  /// Class execution method
  void execute(ProgressAction *progress = nullptr);
  /// Destructor
  virtual ~vtkDataSetToNonOrthogonalDataSet();

private:
  vtkDataSetToNonOrthogonalDataSet &
  operator=(const vtkDataSetToNonOrthogonalDataSet &other);
  vtkDataSetToNonOrthogonalDataSet(
      const vtkDataSetToNonOrthogonalDataSet &other);
  /// Calculate the skew matrix and basis.
  void createSkewInformation(Geometry::OrientedLattice &ol,
                             Kernel::DblMatrix &w,
                             Kernel::Matrix<coord_t> &aff);
  /// Calculate the skew basis vector
  void findSkewBasis(Kernel::V3D &basis, double scale);
  /// Reduce the dimensionality of matrix by 1
  void stripMatrix(Kernel::DblMatrix &mat);
  /// Add the skew basis to metadata
  void updateMetaData(vtkDataSet *ugrid);
  vtkDataSet *m_dataSet; ///< Pointer to VTK dataset to modify
  std::string m_wsName;  ///< The name of the workspace to fetch
  std::size_t m_numDims; ///< Number of dimensions in workspace
  Kernel::DblMatrix
      m_skewMat;         ///< The skew matrix for non-orthogonal representation
  MantidVec m_basisNorm; ///< Holder for the basis normalisation values
  Kernel::V3D m_basisX;  ///< The X direction basis vector
  Kernel::V3D m_basisY;  ///< The Y direction basis vector
  Kernel::V3D m_basisZ;  ///< The Z direction basis vector
  Kernel::SpecialCoordinateSystem
      m_coordType; ///< The coordinate system for the workspace
  std::array<double, 6> m_boundingBox;
  std::unique_ptr<Mantid::VATES::WorkspaceProvider> m_workspaceProvider;
};

} // namespace VATES
} // namespace Mantid

#endif /* MANTID_VATES_VTKDATASETTONONORTHOGONALDATASET_H_ */
