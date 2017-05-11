#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"
#include "MantidKernel/Logger.h"

#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointSet.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkPVChangeOfBasisHelper.h>
#include <vtkInformation.h>

#include <stdexcept>

namespace {
Mantid::Kernel::Logger g_log("vtkDataSetTOScaledDataSet");
}

namespace Mantid {
namespace VATES {
/**
 * Standard constructor for object.
 */
vtkDataSetToScaledDataSet::vtkDataSetToScaledDataSet() = default;
vtkDataSetToScaledDataSet::~vtkDataSetToScaledDataSet() = default;

/**
 * Process the input data. First, scale a copy of the points and apply
 * that to the output data. Next, update the metadata for range information.
 *
 * This is a data source method.
 *
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 * @param inputData : Input point data set to scale
 * @param info : info to obtain the output data set from.
 * @return The resulting scaled dataset
 */
vtkPointSet *vtkDataSetToScaledDataSet::execute(double xScale, double yScale,
                                                double zScale,
                                                vtkPointSet *inputData,
                                                vtkInformation *info) {

  // Extract output dataset from information.
  vtkPointSet *outputData =
      vtkPointSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  return execute(xScale, yScale, zScale, inputData, outputData);
}

/**
 * Process the input data. First, scale a copy of the points and apply
 * that to the output data. Next, update the metadata for range information.
 *
 * This is a data source method.
 *
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 * @param inputData : The dataset to scale
 * @param outputData : The output dataset. Optional. If not specified or null,
 *new one created.
 * @return The resulting scaled dataset
 */
vtkPointSet *vtkDataSetToScaledDataSet::execute(double xScale, double yScale,
                                                double zScale,
                                                vtkPointSet *inputData,
                                                vtkPointSet *outputData) {

  if (!inputData) {
    throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with "
                             "NULL input vtkPointSet");
  }

  if (!outputData) {
    outputData = inputData->NewInstance();
  }

  vtkPoints *points = inputData->GetPoints();

  vtkNew<vtkPoints> newPoints;

  vtkFloatArray *oldPointsArray =
      vtkFloatArray::FastDownCast(points->GetData());
  vtkFloatArray *newPointsArray =
      vtkFloatArray::FastDownCast(newPoints->GetData());

  if (!oldPointsArray || !newPointsArray) {
    throw std::runtime_error("Failed to cast vtkDataArray to vtkFloatArray.");
  } else if (oldPointsArray->GetNumberOfComponents() != 3 ||
             newPointsArray->GetNumberOfComponents() != 3) {
    throw std::runtime_error("points array must have 3 components.");
  }

  // only cast once;
  float Scale[3];
  Scale[0] = static_cast<float>(xScale);
  Scale[1] = static_cast<float>(yScale);
  Scale[2] = static_cast<float>(zScale);
  vtkIdType numberElements = points->GetNumberOfPoints() * 3;
  float *end = oldPointsArray->GetPointer(numberElements);
  float *newPoint = newPointsArray->WritePointer(0, numberElements);
  for (float *oldPoint = oldPointsArray->GetPointer(0); oldPoint < end;
       std::advance(oldPoint, 3), std::advance(newPoint, 3)) {
    newPoint[0] = Scale[0] * oldPoint[0];
    newPoint[1] = Scale[1] * oldPoint[1];
    newPoint[2] = Scale[2] * oldPoint[2];
  }

  // Shallow copy the input.
  outputData->ShallowCopy(inputData);
  // Give the output dataset the scaled set of points.
  outputData->SetPoints(newPoints.GetPointer());

  this->updateMetaData(xScale, yScale, zScale, inputData, outputData);
  return outputData;
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
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 * @param inputData : Input dataset
 * @param outputData : Output dataset
 */
void vtkDataSetToScaledDataSet::updateMetaData(double xScale, double yScale,
                                               double zScale,
                                               vtkPointSet *inputData,
                                               vtkPointSet *outputData) {
  // We need to scale the basis vectors of the input ChangeOfBasis
  // (COB) Matrix and set it as the output COB Matrix.
  auto cobMatrix = vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(inputData);

  vtkVector3d u, v, w;
  if (vtkPVChangeOfBasisHelper::GetBasisVectors(cobMatrix, u, v, w)) {
    u.Set(u.GetX() * xScale, u.GetY() * xScale, u.GetZ() * xScale);
    v.Set(v.GetX() * yScale, v.GetY() * yScale, v.GetZ() * yScale);
    w.Set(w.GetX() * zScale, w.GetY() * zScale, w.GetZ() * zScale);
    cobMatrix = vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(u, v, w);
  } else {
    g_log.warning(
        "Could not extract the basis vectors from the Change-of-Basis-Matrix"
        "data of the scaled data set.\n");
    cobMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    cobMatrix->Identity();
    cobMatrix->Element[0][0] *= xScale;
    cobMatrix->Element[1][1] *= yScale;
    cobMatrix->Element[2][2] *= zScale;
  }

  if (!vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(outputData,
                                                                   cobMatrix)) {
    g_log.warning("The Change-of-Basis-Matrix could not be added to the field "
                  "data of the scaled data set.\n");
  }

  // We also need to update the bounding box for the COB Matrix
  double boundingBox[6];
  inputData->GetBounds(boundingBox);
  if (!vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(outputData,
                                                       boundingBox)) {
    g_log.warning("The bounding box could not be added to the field data of "
                  "the scaled data set.\n");
  }
}

} // namespace VATES
} // namespace Mantid
