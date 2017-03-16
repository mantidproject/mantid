#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"

#include "vtkSMPTools.h"
#include "vtkVector.h"
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPVChangeOfBasisHelper.h>
#include <vtkPointSet.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

#include <vtkPointData.h>
#include "vtkNew.h"

#include <algorithm>
#include <boost/algorithm/string/find.hpp>
#include <stdexcept>

using namespace Mantid;
namespace {

Mantid::Kernel::Logger g_log("vtkDataSetToNonOrthogonalDataSet");

void addChangeOfBasisMatrixToFieldData(
    vtkDataObject *dataObject, const MantidVec &u, const MantidVec &v,
    const MantidVec &w, const std::array<double, 6> &boundingBox) {

  if (!dataObject) {
    throw std::invalid_argument("Change of basis needs a vtkDataObject");
  }
  if (u.size() != 3) {
    throw std::invalid_argument("Change of basis requires 3-element u");
  }
  if (v.size() != 3) {
    throw std::invalid_argument("Change of basis requires 3-element v");
  }
  if (w.size() != 3) {
    throw std::invalid_argument("Change of basis requires 3-element w");
  }

  vtkSmartPointer<vtkMatrix4x4> cobMatrix =
      vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(
          vtkVector3d(&u[0]), vtkVector3d(&v[0]), vtkVector3d(&w[0]));

  if (!vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(dataObject,
                                                                   cobMatrix)) {
    g_log.warning("The Change-of-Basis-Matrix could not be added to the field "
                  "data of the data set.\n");
  }

  if (!vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(dataObject,
                                                       &boundingBox[0])) {
    g_log.warning("The bounding box could not be added to the field data of "
                  "the data set.\n");
  }
}
}

namespace Mantid {
namespace VATES {

/**
 * This is the private class constructor.
 * @param dataset : The VTK data to modify
 * @param name : The MDWorkspace containing the information to construct.
 * @param workspaceProvider: The provider of one or multiple workspaces.
 */
vtkDataSetToNonOrthogonalDataSet::vtkDataSetToNonOrthogonalDataSet(
    vtkDataSet *dataset, std::string name,
    std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider)
    : m_dataSet(dataset), m_wsName(name), m_numDims(3), m_skewMat(),
      m_basisNorm(), m_basisX(1, 0, 0), m_basisY(0, 1, 0), m_basisZ(0, 0, 1),
      m_coordType(Kernel::HKL),
      m_workspaceProvider(std::move(workspaceProvider)) {
  if (!m_dataSet) {
    throw std::runtime_error("Cannot construct "
                             "vtkDataSetToNonOrthogonalDataSet with null VTK "
                             "dataset");
  }
  if (name.empty()) {
    throw std::runtime_error("Cannot construct "
                             "vtkDataSetToNonOrthogonalDataSet without "
                             "associated workspace name");
  }
}

/**
 * Class destructor
 */
vtkDataSetToNonOrthogonalDataSet::~vtkDataSetToNonOrthogonalDataSet() {}

namespace {
struct Worker {
  Mantid::coord_t *m_skew;
  vtkFloatArray *m_pts;
  Worker(Mantid::coord_t *skew, vtkFloatArray *pts)
      : m_skew(skew), m_pts(pts) {}
  void operator()(vtkIdType begin, vtkIdType end) {
    float in[3], out[3];
    for (vtkIdType index = begin; index < end; ++index) {
      m_pts->GetTypedTuple(index, in);
      out[0] = in[0] * m_skew[0] + in[1] * m_skew[1] + in[2] * m_skew[2];
      out[1] = in[0] * m_skew[3] + in[1] * m_skew[4] + in[2] * m_skew[5];
      out[2] = in[0] * m_skew[6] + in[1] * m_skew[7] + in[2] * m_skew[8];
      m_pts->SetTypedTuple(index, out);
    }
  }
};
} // end anon namespace

void vtkDataSetToNonOrthogonalDataSet::execute(ProgressAction *progress) {
  // Downcast to a vtkPointSet
  vtkPointSet *data = vtkPointSet::SafeDownCast(m_dataSet);
  if (!data) {
    throw std::runtime_error("VTK dataset does not inherit from vtkPointSet");
  }

  // Get the workspace from the workspace provider
  API::Workspace_sptr ws = m_workspaceProvider->fetchWorkspace(m_wsName);
  std::string wsType = ws->id();

  Geometry::OrientedLattice oLatt;
  std::vector<double> wMatArr;
  Kernel::Matrix<coord_t> affMat;

  // Have to cast since inherited class doesn't provide access to all info
  if (boost::algorithm::find_first(wsType, "MDHistoWorkspace")) {
    API::IMDHistoWorkspace_const_sptr infoWs =
        boost::dynamic_pointer_cast<const API::IMDHistoWorkspace>(ws);

    m_boundingBox[0] = infoWs->getXDimension()->getMinimum();
    m_boundingBox[1] = infoWs->getXDimension()->getMaximum();
    m_boundingBox[2] = infoWs->getYDimension()->getMinimum();
    m_boundingBox[3] = infoWs->getYDimension()->getMaximum();
    m_boundingBox[4] = infoWs->getZDimension()->getMinimum();
    m_boundingBox[5] = infoWs->getZDimension()->getMaximum();

    m_numDims = infoWs->getNumDims();
    m_coordType = infoWs->getSpecialCoordinateSystem();
    if (Kernel::HKL != m_coordType) {
      throw std::invalid_argument(
          "Cannot create non-orthogonal view for non-HKL coordinates");
    }
    const API::Sample sample = infoWs->getExperimentInfo(0)->sample();
    if (!sample.hasOrientedLattice()) {
      throw std::invalid_argument(
          "OrientedLattice is not present on workspace");
    }
    oLatt = sample.getOrientedLattice();
    const API::Run run = infoWs->getExperimentInfo(0)->run();
    if (!run.hasProperty("W_MATRIX")) {
      throw std::invalid_argument("W_MATRIX is not present on workspace");
    }
    wMatArr = run.getPropertyValueAsType<std::vector<double>>("W_MATRIX");
    try {
      API::CoordTransform const *transform = infoWs->getTransformToOriginal();
      affMat = transform->makeAffineMatrix();
    } catch (std::runtime_error &) {
      // Create identity matrix of dimension+1
      std::size_t nDims = infoWs->getNumDims() + 1;
      Kernel::Matrix<coord_t> temp(nDims, nDims, true);
      affMat = temp;
    }
  }
  // This is only here to make the unit test run.
  if (boost::algorithm::find_first(wsType, "MDEventWorkspace")) {
    API::IMDEventWorkspace_const_sptr infoWs =
        boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(ws);

    m_boundingBox[0] = infoWs->getXDimension()->getMinimum();
    m_boundingBox[1] = infoWs->getXDimension()->getMaximum();
    m_boundingBox[2] = infoWs->getYDimension()->getMinimum();
    m_boundingBox[3] = infoWs->getYDimension()->getMaximum();
    m_boundingBox[4] = infoWs->getZDimension()->getMinimum();
    m_boundingBox[5] = infoWs->getZDimension()->getMaximum();

    m_numDims = infoWs->getNumDims();
    m_coordType = infoWs->getSpecialCoordinateSystem();
    if (Kernel::HKL != m_coordType) {
      throw std::invalid_argument(
          "Cannot create non-orthogonal view for non-HKL coordinates");
    }
    const API::Sample sample = infoWs->getExperimentInfo(0)->sample();
    if (!sample.hasOrientedLattice()) {
      throw std::invalid_argument(
          "OrientedLattice is not present on workspace");
    }
    oLatt = sample.getOrientedLattice();
    const API::Run run = infoWs->getExperimentInfo(0)->run();
    if (!run.hasProperty("W_MATRIX")) {
      throw std::invalid_argument("W_MATRIX is not present on workspace");
    }
    wMatArr = run.getPropertyValueAsType<std::vector<double>>("W_MATRIX");
    try {
      API::CoordTransform const *transform = infoWs->getTransformToOriginal();
      affMat = transform->makeAffineMatrix();
    } catch (std::runtime_error &) {
      // Create identity matrix of dimension+1
      std::size_t nDims = infoWs->getNumDims() + 1;
      Kernel::Matrix<coord_t> temp(nDims, nDims, true);
      affMat = temp;
    }
  }
  Kernel::DblMatrix wTrans(wMatArr);
  this->createSkewInformation(oLatt, wTrans, affMat);

  /// Put together the skew matrix for use
  Mantid::coord_t skew[9];

  // Create from the internal skew matrix
  std::size_t index = 0;
  for (std::size_t i = 0; i < m_skewMat.numRows(); i++) {
    for (std::size_t j = 0; j < m_skewMat.numCols(); j++) {
      skew[index] = static_cast<Mantid::coord_t>(m_skewMat[i][j]);
      index++;
    }
  }

  // Get the original points
  vtkFloatArray *points =
      vtkFloatArray::FastDownCast(data->GetPoints()->GetData());
  if (!points) {
    throw std::runtime_error("Failed to cast vtkDataArray to vtkFloatArray.");
  } else if (points->GetNumberOfComponents() != 3) {
    throw std::runtime_error("points array must have 3 components.");
  }

  Worker func(skew, points);
  if (progress)
    progress->eventRaised(0.67);
  vtkSMPTools::For(0, points->GetNumberOfTuples(), func);
  if (progress)
    progress->eventRaised(1.0);
  this->updateMetaData(data);
}

/**
 * This function will create the skew matrix and basis for a non-orthogonal
 * representation.
 *
 * @param ol : The oriented lattice containing B matrix and crystal basis
 *vectors
 * @param w : The tranform requested when MDworkspace was created
 * @param aff : The affine matrix taking care of coordinate transformations
 */
void vtkDataSetToNonOrthogonalDataSet::createSkewInformation(
    Geometry::OrientedLattice &ol, Kernel::DblMatrix &w,
    Kernel::Matrix<coord_t> &aff) {
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
  for (std::size_t i = 0; i < m_skewMat.numCols(); i++) {
    double sum = 0.0;
    for (std::size_t j = 0; j < m_skewMat.numRows(); j++) {
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
  m_basisNorm = {ol.astar(), ol.bstar(), ol.cstar()};

  // Expand matrix to 4 dimensions if necessary
  if (4 == m_numDims) {
    m_basisNorm.push_back(1.0);
    Kernel::DblMatrix temp(4, 4, true);
    for (std::size_t i = 0; i < 3; i++) {
      for (std::size_t j = 0; j < 3; j++) {
        temp[i][j] = m_skewMat[i][j];
      }
    }
    m_skewMat = temp;
  }

  // Convert affine matrix to similar type as others
  Kernel::DblMatrix affMat(aff.numRows(), aff.numCols());
  for (std::size_t i = 0; i < aff.numRows(); i++) {
    for (std::size_t j = 0; j < aff.numCols(); j++) {
      affMat[i][j] = aff[i][j];
    }
  }
  // Strip affine matrix down to correct dimensions
  this->stripMatrix(affMat);

  // Perform similarity transform to get coordinate orientation correct
  m_skewMat = affMat.Tprime() * (m_skewMat * affMat);
  m_basisNorm = affMat * m_basisNorm;
  if (4 == m_numDims) {
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
                                                     double scale) {
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
void vtkDataSetToNonOrthogonalDataSet::stripMatrix(Kernel::DblMatrix &mat) {
  std::size_t dim = mat.Ssize() - 1;
  Kernel::DblMatrix temp(dim, dim);
  for (std::size_t i = 0; i < dim; i++) {
    for (std::size_t j = 0; j < dim; j++) {
      temp[i][j] = mat[i][j];
    }
  }
  mat = temp;
}

/**
 * This function is responsible for adding the skew basis information to the
 * VTK dataset.
 * @param ugrid : The VTK dataset to add the metadata to
 */
void vtkDataSetToNonOrthogonalDataSet::updateMetaData(vtkDataSet *ugrid) {
  // Create and add the change of basis matrix
  addChangeOfBasisMatrixToFieldData(ugrid, m_basisX, m_basisY, m_basisZ,
                                    m_boundingBox);
}

} // namespace VATES
} // namespace Mantid
