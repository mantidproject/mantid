#include "vtkScaleWorkspace.h"

#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkUnstructuredGrid.h>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkAppendFilter.h>

vtkStandardNewMacro(vtkScaleWorkspace)

using namespace Mantid::VATES;

vtkScaleWorkspace::vtkScaleWorkspace() :
  m_xScaling(1),
  m_yScaling(1),
  m_zScaling(1),
  m_minValue(0.1),
  m_maxValue(0.1),
  m_specialCoordinates(-1),
  m_metadataJsonManager(new MetadataJsonManager()),
  m_vatesConfigurations(new VatesConfigurations())
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkScaleWorkspace::~vtkScaleWorkspace()
{
}


int vtkScaleWorkspace::RequestData(vtkInformation*, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  
  // Try to cast to vktUnstructuredGrid, if this fails then cast it to vtkPolyData
  vtkUnstructuredGrid* inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // This follows the example given here:
  // http://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/PolyDataToUnstructuredGrid
  auto appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
  if (!inputDataSet) {
    auto polyDataSet = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    appendFilter->AddInputData(polyDataSet);
    appendFilter->Update();
    inputDataSet = appendFilter->GetOutput();
  }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *outputDataSet = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSetToScaledDataSet scaler(inputDataSet, outputDataSet);
  scaler.initialize(m_xScaling, m_yScaling, m_zScaling);
  scaler.execute();

  // Need to call an update on the meta data, as it is not guaranteed that RequestInformation will be called
  // before we access the metadata.
  updateMetaData(inputDataSet);
  return 1;
}

int vtkScaleWorkspace::RequestInformation(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // Set the meta data 
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkSmartPointer<vtkUnstructuredGrid> inputDataSet = vtkSmartPointer<vtkUnstructuredGrid>(vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())));
  // This follows the example given here:
  // http://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/PolyDataToUnstructuredGrid
  auto appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
  if (!inputDataSet) {
    auto polyDataSet = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    appendFilter->AddInputData(polyDataSet);
    appendFilter->Update();
    inputDataSet = appendFilter->GetOutput();
  }
  updateMetaData(inputDataSet);
  return 1;
}

void vtkScaleWorkspace::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
Setter for the X Scaling factor
@param xScaling : scaling factor in x
*/
void vtkScaleWorkspace::SetXScaling(double xScaling)
{
  if(xScaling != m_xScaling && xScaling > 0)
  {
    this->Modified();
    m_xScaling = xScaling;
  }
}

/**
Setter for the Y Scaling factor
@param yScaling : scaling factor in y
*/
void vtkScaleWorkspace::SetYScaling(double yScaling)
{
  if(yScaling != m_yScaling && yScaling > 0)
  {
    this->Modified();
    m_yScaling = yScaling;
  }
}

/**
Setter for the Z Scaling factor
@param zScaling : scaling factor in z
*/
void vtkScaleWorkspace::SetZScaling(double zScaling)
{
  if(zScaling != m_zScaling && zScaling > 0)
  {
    this->Modified();
    m_zScaling = zScaling;
  }
}

/**
 * Gets the minimum value of the data associated with the 
 * workspace.
 * @returns The minimum value of the workspace data.
 */
double vtkScaleWorkspace::GetMinValue()
{
  return m_minValue;
}

/**
 * Gets the maximum value of the data associated with the 
 * workspace.
 * @returns The maximum value of the workspace data.
 */
double vtkScaleWorkspace::GetMaxValue()
{
   return m_maxValue;
}

/**
 * Gets the (first) instrument which is associated with the workspace.
 * @return The name of the instrument.
 */
const char* vtkScaleWorkspace::GetInstrument()
{
  return m_instrument.c_str();
}

int vtkScaleWorkspace::GetSpecialCoordinates()
{
  return m_specialCoordinates;
}

/**
 * Update the metadata fields of the plugin based on the information of the inputDataSet
 * @param inputDataSet :: the input data set.
 */
void vtkScaleWorkspace::updateMetaData(vtkUnstructuredGrid *inputDataSet) {
  vtkFieldData* fieldData = inputDataSet->GetFieldData();
  
  // Extract information for meta data in Json format.
  FieldDataToMetadata fieldDataToMetadata;

  std::string jsonString = fieldDataToMetadata(fieldData, m_vatesConfigurations->getMetadataIdJson());
  m_metadataJsonManager->readInSerializedJson(jsonString);

  m_minValue = m_metadataJsonManager->getMinValue();
  m_maxValue = m_metadataJsonManager->getMaxValue();
  m_instrument = m_metadataJsonManager->getInstrument();
  m_specialCoordinates = m_metadataJsonManager->getSpecialCoordinates();
}

/**
 * Set the input types that we expect for this algorithm. These are naturally
 * vtkUnstructredGrid data sets. In order to accomodate for the cut filter's
 * output we need to allow also for vtkPolyData data sets.
 * @param port: the input port
 * @param info: the information object
 * @retuns either success flag (1) or a failure flag (0)
 */
int vtkScaleWorkspace::FillInputPortInformation (int port, vtkInformation *info) {
    // We only have port 0 as an input
    if (port == 0)
    {
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkUnstructuredGrid");
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkPolyData");
      return 1;
    }
  return 0;
}

