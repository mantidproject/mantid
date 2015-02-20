#include "vtkPeaksFilter.h"
#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include <boost/scoped_ptr.hpp>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkUnstructuredGrid.h>

vtkStandardNewMacro(vtkPeaksFilter);

using namespace Mantid::VATES;

vtkPeaksFilter::vtkPeaksFilter() : m_peaksWorkspaceName(""),
                                   m_radiusNoShape(0.5),
                                   m_radiusType(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkPeaksFilter::~vtkPeaksFilter()
{
}


int vtkPeaksFilter::RequestData(vtkInformation*, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *outputDataSet = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the peaks workspace
  if (!Mantid::API::AnalysisDataService::Instance().doesExist(m_peaksWorkspaceName))
  {
  return 0;
  }

  Mantid::API::IPeaksWorkspace_sptr peaksWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IPeaksWorkspace>(m_peaksWorkspaceName);

  vtkDataSetToPeaksFilteredDataSet peaksFilter(inputDataSet, outputDataSet);
  peaksFilter.initialize(peaksWorkspace, m_radiusNoShape, m_radiusType);
  peaksFilter.execute();

  return 1;
}

int vtkPeaksFilter::RequestInformation(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // Set the meta data 
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkFieldData* fieldData = inputDataSet->GetFieldData();
  
  // Extract information for meta data in Json format.
  FieldDataToMetadata fieldDataToMetadata;

#if 0
  std::string jsonString = fieldDataToMetadata(fieldData, m_vatesConfigurations->getMetadataIdJson());
  m_metadataJsonManager->readInSerializedJson(jsonString);

  m_minValue = m_metadataJsonManager->getMinValue();
  m_maxValue = m_metadataJsonManager->getMaxValue();
  m_instrument = m_metadataJsonManager->getInstrument();
#endif
  return 1;
}

void vtkPeaksFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
 * Set the peaks workspace name
 * @param peaksWorkspaceName The peaks workspace name.
*/
void vtkPeaksFilter::SetPeaksWorkspace(std::string peaksWorkspaceName)
{
  m_peaksWorkspaceName = peaksWorkspaceName;
}

/**
 * Set the radius for PeakShape == NoShape.
 * @param radius The radius
 */
void vtkPeaksFilter::SetRadiusNoShape(double radius)
{
  m_radiusNoShape = radius;
  this->Modified();
}

/**
 * Set the radius type.
 * @param type The type of the radius
 */
void vtkPeaksFilter::SetRadiusType(int type)
{
  m_radiusType = type;
  this->Modified();
}
