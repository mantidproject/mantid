#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"
#include "MantidKernel/Logger.h"

#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkPVChangeOfBasisHelper.h>

#include <stdexcept>


namespace {
Mantid::Kernel::Logger g_log("vtkDataSetTOScaledDataSet");
}

namespace Mantid {
namespace VATES {
/**
 * Standard constructor for object.
 * @param input : The dataset to scale
 * @param output : The resulting scaled dataset
 */
vtkDataSetToScaledDataSet::vtkDataSetToScaledDataSet(
    vtkUnstructuredGrid *input, vtkUnstructuredGrid *output)
    : m_inputData(input), m_outputData(output), m_xScaling(1.0),
      m_yScaling(1.0), m_zScaling(1.0), m_isInitialised(false) {
  if (NULL == m_inputData) {
    throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with "
                             "NULL input vtkUnstructuredGrid");
  }
  if (NULL == m_outputData) {
    throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with "
                             "NULL output vtkUnstructuredGrid");
  }
}

vtkDataSetToScaledDataSet::~vtkDataSetToScaledDataSet() {}

/**
 * Set the scaling factors for the data, once run, the object is now
 * initialised.
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 */
void vtkDataSetToScaledDataSet::initialize(double xScale, double yScale,
                                           double zScale) {
  m_xScaling = xScale;
  m_yScaling = yScale;
  m_zScaling = zScale;
  m_isInitialised = true;
}

/**
 * Process the input data. First, scale a copy of the points and apply
 * that to the output data. Next, update the metadata for range information.
 */
void vtkDataSetToScaledDataSet::execute() {
  if (!m_isInitialised) {
    throw std::runtime_error(
        "vtkDataSetToScaledDataSet needs initialize run before executing");
  }

  vtkPoints *points = m_inputData->GetPoints();

  double *point;
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(points->GetNumberOfPoints());
  for (int i = 0; i < points->GetNumberOfPoints(); i++) {
    point = points->GetPoint(i);
    point[0] *= m_xScaling;
    point[1] *= m_yScaling;
    point[2] *= m_zScaling;
    newPoints->InsertNextPoint(point);
  }
  // Shallow copy the input.
  m_outputData->ShallowCopy(m_inputData);
  // Give the output dataset the scaled set of points.
  m_outputData->SetPoints(newPoints);
  this->updateMetaData();
}

/**
 * In order for the axis range and labels to not come out scaled,
 * this function set metadata that ParaView will read to override
 * the scaling to return the original presentation.
 * See
 * http://www.paraview.org/ParaQ/Doc/Nightly/html/classvtkCubeAxesRepresentation.html
 * and
 * http://www.paraview.org/ParaView/Doc/Nightly/www/cxx-doc/classvtkPVChangeOfBasisHelper.html
 * for a better understanding.
 */
void vtkDataSetToScaledDataSet::updateMetaData() {
  // We need to put the scaling on the diagonal elements of the ChangeOfBasis
  // (COB) Matrix.
  vtkSmartPointer<vtkMatrix4x4> cobMatrix =
      vtkSmartPointer<vtkMatrix4x4>::New();
  cobMatrix->Identity();
  cobMatrix->Element[0][0] *= m_xScaling;
  cobMatrix->Element[1][1] *= m_yScaling;
  cobMatrix->Element[2][2] *= m_zScaling;

  if (!vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(m_outputData,
                                                                   cobMatrix)) {
    g_log.warning("The Change-of-Basis-Matrix could not be added to the field "
                  "data of the scaled data set.\n");
  }

  // We also need to update the bounding box for the COB Matrix
  double boundingBox[6];
  m_inputData->GetBounds(boundingBox);
  if (!vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(m_outputData,
                                                       boundingBox)) {
    g_log.warning("The bounding box could not be added to the field data of "
                  "the scaled data set.\n");
  }
}

} // namespace VATES
} // namespace Mantid
