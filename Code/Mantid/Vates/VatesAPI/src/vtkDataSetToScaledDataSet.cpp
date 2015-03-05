#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"

#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

#include <stdexcept>

namespace Mantid
{
namespace VATES
{
  /**
   * Standard constructor for object.
   * @param input : The dataset to scale
   * @param output : The resulting scaled dataset
   */
  vtkDataSetToScaledDataSet::vtkDataSetToScaledDataSet(vtkUnstructuredGrid *input,
                                                       vtkUnstructuredGrid *output) :
    m_inputData(input),
    m_outputData(output),
    m_xScaling(1.0),
    m_yScaling(1.0),
    m_zScaling(1.0),
    m_isInitialised(false)
  {
    if (NULL == m_inputData)
    {
      throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with NULL input vtkUnstructuredGrid");
    }
    if (NULL == m_outputData)
    {
      throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with NULL output vtkUnstructuredGrid");
    }
  }
    
  vtkDataSetToScaledDataSet::~vtkDataSetToScaledDataSet()
  {
  }
  
  /**
   * Set the scaling factors for the data, once run, the object is now
   * initialised.
   * @param xScale : Scale factor for the x direction
   * @param yScale : Scale factor for the y direction
   * @param zScale : Scale factor for the z direction
   */
  void vtkDataSetToScaledDataSet::initialize(double xScale, double yScale, double zScale)
  {
    m_xScaling = xScale;
    m_yScaling = yScale;
    m_zScaling = zScale;
    m_isInitialised = true;
  }

  /**
   * Process the input data. First, scale a copy of the points and apply
   * that to the output data. Next, update the metadata for range information.
   */
  void vtkDataSetToScaledDataSet::execute()
  {
    if (!m_isInitialised)
    {
      throw std::runtime_error("vtkDataSetToScaledDataSet needs initialize run before executing");
    }

    vtkPoints *points = m_inputData->GetPoints();

    double *point;
    vtkPoints* newPoints = vtkPoints::New();
    newPoints->Allocate(points->GetNumberOfPoints());
    for(int i = 0; i < points->GetNumberOfPoints(); i++)
    {
      point = points->GetPoint(i);
      point[0] *= m_xScaling;
      point[1] *= m_yScaling;
      point[2] *= m_zScaling;
      newPoints->InsertNextPoint(point);
    }
    //Shallow copy the input.
    m_outputData->ShallowCopy(m_inputData);
    //Give the output dataset the scaled set of points.
    m_outputData->SetPoints(newPoints);
    this->updateMetaData();
  }

  /**
   * In order for the axis range and labels to not come out scaled,
   * this function set metadata that ParaView will read to override
   * the scaling to return the original presentation.
   */
  void vtkDataSetToScaledDataSet::updateMetaData()
  {
    // Grab original bounding box so we can recall original extents
    double bb[6];
    m_inputData->GetBounds(bb);

    vtkFieldData *fieldData = m_outputData->GetFieldData();

    // Set that the label range is actively being managed
    vtkNew<vtkUnsignedCharArray> activeLabelRange;
    activeLabelRange->SetNumberOfComponents(1);
    activeLabelRange->SetNumberOfTuples(3);
    activeLabelRange->SetName("LabelRangeActiveFlag");
    fieldData->AddArray(activeLabelRange.GetPointer());
    activeLabelRange->SetValue(0, 1);
    activeLabelRange->SetValue(1, 1);
    activeLabelRange->SetValue(2, 1);

    // Set the original axis extents from the bounding box
    vtkNew<vtkFloatArray> uLabelRange;
    uLabelRange->SetNumberOfComponents(2);
    uLabelRange->SetNumberOfTuples(1);
    uLabelRange->SetName("LabelRangeForX");
    double labelRangeX[2] = {bb[0], bb[1]};
    uLabelRange->SetTuple(0, labelRangeX);
    fieldData->AddArray(uLabelRange.GetPointer());

    vtkNew<vtkFloatArray> vLabelRange;
    vLabelRange->SetNumberOfComponents(2);
    vLabelRange->SetNumberOfTuples(1);
    vLabelRange->SetName("LabelRangeForY");
    double labelRangeY[2] = {bb[2], bb[3]};
    vLabelRange->SetTuple(0, labelRangeY);
    fieldData->AddArray(vLabelRange.GetPointer());

    vtkNew<vtkFloatArray> wLabelRange;
    wLabelRange->SetNumberOfComponents(2);
    wLabelRange->SetNumberOfTuples(1);
    wLabelRange->SetName("LabelRangeForZ");
    double labelRangeZ[2] = {bb[4], bb[5]};
    wLabelRange->SetTuple(0, labelRangeZ);
    fieldData->AddArray(wLabelRange.GetPointer());

    // Set the linear transform for each axis based on scaling factor
    vtkNew<vtkFloatArray> uLinearTransform;
    uLinearTransform->SetNumberOfComponents(2);
    uLinearTransform->SetNumberOfTuples(1);
    uLinearTransform->SetName("LinearTransformForX");
    double transformX[2] = {1.0 / m_xScaling, 0.0};
    uLinearTransform->SetTuple(0, transformX);
    fieldData->AddArray(uLinearTransform.GetPointer());

    vtkNew<vtkFloatArray> vLinearTransform;
    vLinearTransform->SetNumberOfComponents(2);
    vLinearTransform->SetNumberOfTuples(1);
    vLinearTransform->SetName("LinearTransformForY");
    double transformY[2] = {1.0 / m_yScaling, 0.0};
    vLinearTransform->SetTuple(0, transformY);
    fieldData->AddArray(vLinearTransform.GetPointer());

    vtkNew<vtkFloatArray> wLinearTransform;
    wLinearTransform->SetNumberOfComponents(2);
    wLinearTransform->SetNumberOfTuples(1);
    wLinearTransform->SetName("LinearTransformForZ");
    double transformZ[2] = {1.0 / m_zScaling, 0.0};
    wLinearTransform->SetTuple(0, transformZ);
    fieldData->AddArray(wLinearTransform.GetPointer());
  }

} // namespace VATES
} // namespace Mantid
