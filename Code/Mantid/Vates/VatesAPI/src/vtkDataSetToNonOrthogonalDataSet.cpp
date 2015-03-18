#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/Crystal/UnitCell.h"
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

/**
 * This function constructs and executes the helper class.
 * @param dataset : The VTK data to modify
 * @param name : The MDWorkspace containing the information to construct modification
 */
void vtkDataSetToNonOrthogonalDataSet::exec(vtkDataSet *dataset, std::string name)
{
  vtkDataSetToNonOrthogonalDataSet temp(dataset, name);
  temp.execute();
}

/**
 * This is the private class constructor.
 * @param dataset : The VTK data to modify
 * @param name : The MDWorkspace containing the information to construct modification
 */
vtkDataSetToNonOrthogonalDataSet::vtkDataSetToNonOrthogonalDataSet(vtkDataSet *dataset,
                                                                   std::string name) :
  m_dataSet(dataset),
  m_wsName(name),
  m_hc(0),
  m_numDims(3),
  m_skewMat(),
  m_basisNorm(),
  m_basisX(1, 0, 0),
  m_basisY(0, 1, 0),
  m_basisZ(0, 0, 1),
  m_coordType(Kernel::HKL)
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
  vtkPointSet *data = vtkPointSet::SafeDownCast(m_dataSet);
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

    Geometry::OrientedLattice oLatt;
    std::vector<double> wMatArr;
    Kernel::Matrix<coord_t> affMat;

    // Have to cast since inherited class doesn't provide access to all info
    if (boost::algorithm::find_first(wsType, "MDHistoWorkspace"))
    {
      API::IMDHistoWorkspace_const_sptr infoWs = boost::dynamic_pointer_cast<const API::IMDHistoWorkspace>(ws);
      m_numDims = infoWs->getNumDims();
      m_coordType = infoWs->getSpecialCoordinateSystem();
      if (Kernel::HKL != m_coordType)
      {
        throw std::invalid_argument("Cannot create non-orthogonal view for non-HKL coordinates");
      }
      const API::Sample sample = infoWs->getExperimentInfo(0)->sample();
      if (!sample.hasOrientedLattice())
      {
        throw std::invalid_argument("OrientedLattice is not present on workspace");
      }
      oLatt = sample.getOrientedLattice();
      const API::Run run = infoWs->getExperimentInfo(0)->run();
      if (!run.hasProperty("W_MATRIX"))
      {
        throw std::invalid_argument("W_MATRIX is not present on workspace");
      }
      wMatArr = run.getPropertyValueAsType<std::vector<double > >("W_MATRIX");
      try
      {
        API::CoordTransform *transform = infoWs->getTransformToOriginal();
        affMat = transform->makeAffineMatrix();
      }
      catch (std::runtime_error &)
      {
        // Create identity matrix of dimension+1
        std::size_t nDims = infoWs->getNumDims() + 1;
        Kernel::Matrix<coord_t> temp(nDims, nDims, true);
        affMat = temp;
      }
    }
    // This is only here to make the unit test run.
    if (boost::algorithm::find_first(wsType, "MDEventWorkspace"))
    {
      API::IMDEventWorkspace_const_sptr infoWs = boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(ws);
      m_numDims = infoWs->getNumDims();
      m_coordType = infoWs->getSpecialCoordinateSystem();
      if (Kernel::HKL != m_coordType)
      {
        throw std::invalid_argument("Cannot create non-orthogonal view for non-HKL coordinates");
      }
      const API::Sample sample = infoWs->getExperimentInfo(0)->sample();
      if (!sample.hasOrientedLattice())
      {
        throw std::invalid_argument("OrientedLattice is not present on workspace");
      }
      oLatt = sample.getOrientedLattice();
      const API::Run run = infoWs->getExperimentInfo(0)->run();
      if (!run.hasProperty("W_MATRIX"))
      {
        throw std::invalid_argument("W_MATRIX is not present on workspace");
      }
      wMatArr = run.getPropertyValueAsType<std::vector<double > >("W_MATRIX");
      try
      {
        API::CoordTransform *transform = infoWs->getTransformToOriginal();
        affMat = transform->makeAffineMatrix();
      }
      catch (std::runtime_error &)
      {
        // Create identity matrix of dimension+1
        std::size_t nDims = infoWs->getNumDims() + 1;
        Kernel::Matrix<coord_t> temp(nDims, nDims, true);
        affMat = temp;
      }
    }
    Kernel::DblMatrix wTrans(wMatArr);
    this->createSkewInformation(oLatt, wTrans, affMat);
  }

  // Get the original points
  vtkPoints *points = data->GetPoints();
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
    skew[0] = 1.0;
    skew[1] = 0.0;
    skew[2] = 0.0;
    skew[3] = 0.0;
    skew[4] = 1.0;
    skew[5] = 0.0;
    skew[6] = 0.0;
    skew[7] = 0.0;
    skew[8] = 1.0;
    break;
  case 3:
    // Gd2, HEKL, a scaled
    skew[0] = 1.0;
    skew[1] = 0.0;
    skew[2] = -0.65465367;
    skew[3] = 0.0;
    skew[4] = 1.0;
    skew[5] = 0.0;
    skew[6] = 0.0;
    skew[7] = 0.0;
    skew[8] = 0.75592895;
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
    double *inPoint = points->GetPoint(i);
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
  // Get the B matrix
  Kernel::DblMatrix bMat = ol.getB();
  // Apply the W tranform matrix
  bMat *= w;
  // Create G*
  Kernel::DblMatrix gStar = bMat.Tprime() * bMat;
  Geometry::UnitCell uc(ol);
  uc.recalculateFromGstar(gStar);
  m_skewMat = uc.getB();
  // Calculate the column normalisation
  std::vector<double> bNorm;
  for (std::size_t i = 0; i < m_skewMat.numCols(); i++)
  {
    double sum = 0.0;
    for (std::size_t j = 0; j < m_skewMat.numRows(); j++)
    {
      sum += m_skewMat[j][i] * m_skewMat[j][i];
    }
    bNorm.push_back(std::sqrt(sum));
  }
  // Apply column normalisation to skew matrix
  Kernel::DblMatrix scaleMat(3, 3, true);
  scaleMat[0][0] /= bNorm[0];
  scaleMat[1][1] /= bNorm[1];
  scaleMat[2][2] /= bNorm[2];
  m_skewMat *= scaleMat;

  // Setup basis normalisation array
  // Intel and MSBuild can't handle this
  //m_basisNorm = {ol.astar(), ol.bstar(), ol.cstar()};
  m_basisNorm.push_back(ol.astar());
  m_basisNorm.push_back(ol.bstar());
  m_basisNorm.push_back(ol.cstar());

  // Expand matrix to 4 dimensions if necessary
  if (4 == m_numDims)
  {
    m_basisNorm.push_back(1.0);
    Kernel::DblMatrix temp(4, 4, true);
    for (std::size_t i = 0; i < 3; i++)
    {
      for (std::size_t j = 0; j < 3; j++)
      {
        temp[i][j] = m_skewMat[i][j];
      }
    }
    m_skewMat = temp;
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
  m_skewMat = affMat.Tprime() * (m_skewMat * affMat);
  m_basisNorm = affMat * m_basisNorm;
  if (4 == m_numDims)
  {
    this->stripMatrix(m_skewMat);
  }

  this->findSkewBasis(m_basisX, m_basisNorm[0]);
  this->findSkewBasis(m_basisY, m_basisNorm[1]);
  this->findSkewBasis(m_basisZ, m_basisNorm[2]);
}

/**
 * This function calculates the given skew basis vector.
 *
 * @param basis : The "base" basis vector.
 * @param scale : Scale factor for the basis vector.
 */
void vtkDataSetToNonOrthogonalDataSet::findSkewBasis(Kernel::V3D &basis,
                                                     double scale)
{
  basis = m_skewMat * basis;
  basis /= scale;
  basis.normalize();
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

/**
 * This function copies a vector to a C array.
 *
 * @param arr : The array to copy into
 * @param vec : The vector to copy from
 */
void vtkDataSetToNonOrthogonalDataSet::copyToRaw(double *arr, MantidVec vec)
{
  for (std::size_t i = 0; i < vec.size(); i++)
  {
    arr[i] = vec[i];
  }
}

/**
 * This function is responsible for adding the skew basis information to the
 * VTK dataset.
 * @param ugrid : The VTK dataset to add the metadata to
 */
void vtkDataSetToNonOrthogonalDataSet::updateMetaData(vtkDataSet *ugrid)
{
  double baseX[3];
  double baseY[3];
  double baseZ[3];
  switch (m_hc)
  {
  case 1:
    // Gd, HEKL
    baseX[0] = 1.0;
    baseX[1] = 0.0;
    baseX[2] = 0.0;
    baseY[0] = 0.0;
    baseY[1] = 1.0;
    baseY[2] = 0.0;
    baseZ[0] = 0.5;
    baseZ[1] = 0.0;
    baseZ[2] = 0.8660254;
    break;
  case 2:
    // Gd2, HEKL
    baseX[0] = 1.0;
    baseX[1] = 0.0;
    baseX[2] = 0.0;
    baseY[0] = 0.0;
    baseY[1] = 1.0;
    baseY[2] = 0.0;
    baseZ[0] = 0.0;
    baseZ[1] = 0.0;
    baseZ[2] = 1.0;
    break;
  case 3:
    // Gd2, HEKL, a scaled
    baseX[0] = 1.0;
    baseX[1] = 0.0;
    baseX[2] = 0.0;
    baseY[0] = 0.0;
    baseY[1] = 1.0;
    baseY[2] = 0.0;
    baseZ[0] = -0.65465367;
    baseZ[1] = 0.0;
    baseZ[2] = 0.75592895;
    break;
  default:
    // Create from the internal basis vectors
    this->copyToRaw(baseX, m_basisX);
    this->copyToRaw(baseY, m_basisY);
    this->copyToRaw(baseZ, m_basisZ);
    break;
  }

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
