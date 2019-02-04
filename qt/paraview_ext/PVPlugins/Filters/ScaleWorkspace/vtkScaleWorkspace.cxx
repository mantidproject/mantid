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



vtkStandardNewMacro(vtkScaleWorkspace)

using namespace Mantid::VATES;

vtkScaleWorkspace::vtkScaleWorkspace() :
  m_xScaling(1),
  m_yScaling(1),
  m_zScaling(1),
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
 vtkSmartPointer<vtkPointSet> inputDataSet = vtkSmartPointer<vtkPointSet>(vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSetToScaledDataSet scaler;
  scaler.execute(m_xScaling, m_yScaling, m_zScaling, inputDataSet, outInfo);

  // Need to call an update on the meta data, as it is not guaranteed that RequestInformation will be called
  // before we access the metadata.
  updateMetaData(inputDataSet);
  return 1;
}

int vtkScaleWorkspace::RequestInformation(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // Set the meta data 
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkSmartPointer<vtkPointSet> inputDataSet = vtkSmartPointer<vtkPointSet>(vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())));

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
void vtkScaleWorkspace::updateMetaData(vtkPointSet *inputDataSet) {
  vtkFieldData* fieldData = inputDataSet->GetFieldData();
  
  // Extract information for meta data in Json format.
  FieldDataToMetadata fieldDataToMetadata;

  std::string jsonString = fieldDataToMetadata(fieldData, m_vatesConfigurations->getMetadataIdJson());
  m_metadataJsonManager->readInSerializedJson(jsonString);

  m_instrument = m_metadataJsonManager->getInstrument();
  m_specialCoordinates = m_metadataJsonManager->getSpecialCoordinates();
}

/**
 * Set the input types that we expect for this algorithm. These are naturally
 * vtkUnstructredGrid data sets. In order to accomodate for the cut filter's
 * output we need to allow also for vtkPolyData data sets.
 * @retuns either success flag (1) or a failure flag (0)
 */
int vtkScaleWorkspace::FillInputPortInformation (int, vtkInformation *) {
  return 0;
}

