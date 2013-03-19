#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"

#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include <boost/algorithm/string/find.hpp>
#include <stdexcept>

namespace Mantid
{
namespace VATES
{

void vtkDataSetToNonOrthogonalDataSet::exec(vtkDataSet *dataset, std::string name)
{
  vtkDataSetToNonOrthogonalDataSet temp(dataset, name);
  temp.execute();
}

vtkDataSetToNonOrthogonalDataSet::vtkDataSetToNonOrthogonalDataSet(vtkDataSet *dataset,
                                                                   std::string name) :
  m_dataSet(dataset), m_wsName(name), m_hc(1), m_numDims(3), m_skewMat()
{
  if (NULL == m_dataSet)
  {
    throw std::runtime_error("Cannot construct vtkDataSetToNonOrthogonalDataSet with null VTK dataset");
  }
  if (name.empty())
  {
    throw std::runtime_error("Cannot construct vtkDataSetToNonOrthogonalDataSet without associated workspace name");
  }
}

/**
 * Class destructor
 */
vtkDataSetToNonOrthogonalDataSet::~vtkDataSetToNonOrthogonalDataSet()
{
}

void vtkDataSetToNonOrthogonalDataSet::execute()
{
  // Downcast to a vtkUnstructuredGrid
  vtkUnstructuredGrid *data = vtkUnstructuredGrid::SafeDownCast(m_dataSet);
  if (NULL == data)
  {
    throw std::runtime_error("VTK dataset does not inherit from vtkPointSet");
  }

  if (0 == m_hc)
  {
    // Get the workspace from the ADS
    ADSWorkspaceProvider<API::IMDWorkspace> workspaceProvider;
    API::Workspace_sptr ws = workspaceProvider.fetchWorkspace(m_wsName);
    std::string wsType = ws->id();
    // Have to cast since inherited class doesn't provide access to all info
    if (boost::algorithm::find_first(wsType, "MDEventWorkspace"))
    {
      API::IMDEventWorkspace_const_sptr infoWs = boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(ws);
      m_numDims = infoWs->getNumDims();
      Geometry::OrientedLattice oLatt = infoWs->getExperimentInfo(0)->sample().getOrientedLattice();
      Kernel::DblMatrix wTrans = infoWs->getWTransf();
      API::CoordTransform *transform = infoWs->getTransformToOriginal();
      Kernel::Matrix<coord_t> affMat = transform->makeAffineMatrix();
      this->createSkewInformation(oLatt, wTrans, affMat);
    }
  }

  // Get the original points
  vtkPoints *points = data->GetPoints();
  double *inPoint;
  double outPoint[3];
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(points->GetNumberOfPoints());

  /// Put together the skew matrix for use
  double skew[9];
  switch (m_hc)
  {
  case 1:
    // Gd, HEKL
    skew[0] = 1.0;
    skew[1] = 0.0;
    skew[2] = 0.5;
    skew[3] = 0.0;
    skew[4] = 1.0;
    skew[5] = 0.0;
    skew[6] = 0.0;
    skew[7] = 0.0;
    skew[8] = 0.8660254;
    break;
  case 2:
    // Gd2, HEKL
    skew[0] = 0.8660254;
    skew[1] = 0.0;
    skew[2] = 0.5;
    skew[3] = 0.0;
    skew[4] = 1.0;
    skew[5] = 0.0;
    skew[6] = 0.5;
    skew[7] = 0.0;
    skew[8] = -0.8660254;
    break;
  default:
    // Create from the internal skew matrix
    std::size_t index = 0;
    for (std::size_t i = 0; i < m_skewMat.numRows(); i++)
    {
      for(std::size_t j = 0; j < m_skewMat.numCols(); j++)
      {
        skew[index] = m_skewMat[i][j];
        index++;
      }
    }
    break;
  }

  for(int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    inPoint = points->GetPoint(i);
    vtkMatrix3x3::MultiplyPoint(skew, inPoint, outPoint);
    newPoints->InsertNextPoint(outPoint);
  }
  data->SetPoints(newPoints);
  this->updateMetaData(data);
}

/**
 * This function will create the skew matrix and basis for a non-orthogonal
 * representation.
 *
 * @param ol : The oriented lattice containing B matrix and crystal basis vectors
 * @param w : The tranform requested when MDworkspace was created
 * @param aff : The affine matrix taking care of coordinate transformations
 */
void vtkDataSetToNonOrthogonalDataSet::createSkewInformation(Geometry::OrientedLattice &ol,
                                                             Kernel::DblMatrix &w,
                                                             Kernel::Matrix<coord_t> &aff)
{
  // Get and scale the B matrix
  Kernel::DblMatrix bMat = ol.getB();
  Kernel::DblMatrix scaleMat(3, 3, true);
  scaleMat[0][0] /= ol.astar();
  scaleMat[1][1] /= ol.bstar();
  scaleMat[2][2] /= ol.cstar();
  bMat *= scaleMat;
  // Apply the W tranform matrix
  bMat *= w;

  // Expand matrix to 4 dimensions if necessary
  if (4 == m_numDims)
  {
    Kernel::DblMatrix temp(4, 4, true);
    for (std::size_t i = 0; i < 3; i++)
    {
      for (std::size_t j = 0; j < 3; j++)
      {
        temp[i][j] = bMat[i][j];
      }
    }
    bMat = temp;
  }

  // Convert affine matrix to similar type as others
  Kernel::DblMatrix affMat(aff.numRows(), aff.numCols());
  for (std::size_t i = 0; i < aff.numRows(); i++)
  {
    for (std::size_t j = 0; j < aff.numCols(); j++)
    {
      affMat[i][j] = aff[i][j];
    }
  }
  // Strip affine matrix down to correct dimensions
  this->stripMatrix(affMat);

  // Perform similarity transform to get coordinate orientation correct
  bMat *= affMat;
  m_skewMat = affMat.Transpose() * bMat;
  if (4 == m_numDims)
  {
    this->stripMatrix(m_skewMat);
  }
}

/**
 * This function takes a square matrix and reduces its dimensionality by
 * one.
 *
 * @param mat : The matrix to strip dimensionality
 */
void vtkDataSetToNonOrthogonalDataSet::stripMatrix(Kernel::DblMatrix &mat)
{
  std::size_t dim = mat.Ssize() - 1;
  Kernel::DblMatrix temp(dim, dim);
  for (std::size_t i = 0; i < dim; i++)
  {
    for (std::size_t j = 0; j < dim; j++)
    {
      temp[i][j] = mat[i][j];
    }
  }
  mat = temp;
}

void vtkDataSetToNonOrthogonalDataSet::updateMetaData(vtkUnstructuredGrid *ugrid)
{
  /*
  // Gd2, HKLE
  double baseX[3] = {0.8660254, 0.5, 0.0};
  double baseY[3] = {0.5, -0.8660254, 0.0};
  double baseZ[3] = {0.0, 0.0, 1.0};
  */
  /*
  // Gd, HKLE
  double baseX[3] = {1.0, 0.0, 0.0};
  double baseY[3] = {0.5, 0.8660254, 0.0};
  double baseZ[3] = {0.0, 0.0, 1.0};
  */

  // Gd, HEKL
  double baseX[3] = {1.0, 0.0, 0.0};
  double baseY[3] = {0.0, 1.0, 0.0};
  double baseZ[3] = {0.5, 0.0, 0.8660254};

  /*
  // Gd2, HEKL
  double baseX[3] = {0.8660254, 0.0, 0.5};
  double baseY[3] = {0.0, 1.0, 0.0};
  double baseZ[3] = {0.5, 0.0, -0.8660254};
  */

  vtkFieldData *fieldData = ugrid->GetFieldData();

  vtkNew<vtkFloatArray> uBase;
  uBase->SetNumberOfComponents(3);
  uBase->SetNumberOfTuples(1);
  uBase->SetName("AxisBaseForX");
  uBase->SetTuple(0, baseX);
  fieldData->AddArray(uBase.GetPointer());

  vtkNew<vtkFloatArray> vBase;
  vBase->SetNumberOfComponents(3);
  vBase->SetNumberOfTuples(1);
  vBase->SetName("AxisBaseForY");
  vBase->SetTuple(0, baseY);
  fieldData->AddArray(vBase.GetPointer());

  vtkNew<vtkFloatArray> wBase;
  wBase->SetNumberOfComponents(3);
  wBase->SetNumberOfTuples(1);
  wBase->SetName("AxisBaseForZ");
  wBase->SetTuple(0, baseZ);
  fieldData->AddArray(wBase.GetPointer());
}

} // namespace VATES
} // namespace Mantid
