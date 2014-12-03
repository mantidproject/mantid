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

vtkStandardNewMacro(vtkScaleWorkspace);

using namespace Mantid::VATES;

vtkScaleWorkspace::vtkScaleWorkspace() :
  m_xScaling(1),
  m_yScaling(1),
  m_zScaling(1),
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
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *outputDataSet = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSetToScaledDataSet scaler(inputDataSet, outputDataSet);
  scaler.initialize(m_xScaling, m_yScaling, m_zScaling);
  scaler.execute();
  return 1;
}

int vtkScaleWorkspace::RequestInformation(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // Set the meta data 
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkFieldData* fieldData = inputDataSet->GetFieldData();
  
  // Extract information for meta data in Json format.
  FieldDataToMetadata fieldDataToMetadata;

  std::string jsonString = fieldDataToMetadata(fieldData, m_vatesConfigurations->getMetadataIdJson());
  m_metadataJsonManager->readInSerializedJson(jsonString);

  m_minValue = m_metadataJsonManager->getMinValue();
  m_maxValue = m_metadataJsonManager->getMaxValue();
  m_instrument = m_metadataJsonManager->getInstrument();

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
 * @return The minimum value of the workspace data.
 */
double vtkScaleWorkspace::GetMinValue()
{
  return m_minValue;
}

/**
 * Gets the maximum value of the data associated with the 
 * workspace.
 * @return The maximum value of the workspace data.
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