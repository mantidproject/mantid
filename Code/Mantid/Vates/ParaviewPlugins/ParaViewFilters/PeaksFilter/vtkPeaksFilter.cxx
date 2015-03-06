#include "vtkPeaksFilter.h"
#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
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

vtkPeaksFilter::vtkPeaksFilter() : m_peaksWorkspaceNames(""),
                                   m_delimiter(";"),
                                   m_radiusNoShape(0.5),
                                   m_radiusType(0),
                                   m_minValue(0.1),
                                   m_maxValue(0.1),
                                   m_metadataJsonManager(new MetadataJsonManager()),
                                   m_vatesConfigurations(new VatesConfigurations())
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

#if 1
  std::vector<std::string> peaksWorkspaceNames = extractPeakWorkspaceNames();
  std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksWorkspaces = getPeaksWorkspaces(peaksWorkspaceNames);

  if (peaksWorkspaces.empty())
  {
    return 0;
  }
  FilterUpdateProgressAction<vtkPeaksFilter> drawingProgressUpdate(this, "Drawing...");

  vtkDataSetToPeaksFilteredDataSet peaksFilter(inputDataSet, outputDataSet);
  peaksFilter.initialize(peaksWorkspaces, m_radiusNoShape, m_radiusType);
  peaksFilter.execute(drawingProgressUpdate);
#else
  outputDataSet->ShallowCopy(inputDataSet);
#endif
  return 1;
}

int vtkPeaksFilter::RequestInformation(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // Set the meta data 
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkUnstructuredGrid *inputDataSet = vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If the field data does not contain the metadata, then don't do anything.
  try
  {
    vtkFieldData* fieldData = inputDataSet->GetFieldData();
  
    // Extract information for meta data in Json format.
    FieldDataToMetadata fieldDataToMetadata;

    std::string jsonString = fieldDataToMetadata(fieldData, m_vatesConfigurations->getMetadataIdJson());
    m_metadataJsonManager->readInSerializedJson(jsonString);

    m_minValue = m_metadataJsonManager->getMinValue();
    m_maxValue = m_metadataJsonManager->getMaxValue();
    m_instrument = m_metadataJsonManager->getInstrument();
  }
  catch (...)
  {
  }

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
  m_peaksWorkspaceNames = peaksWorkspaceName;
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

/** 
 * Updates the progress bar.
 * @param progress Progress indicator.
 * @param message Progress message.
 */
void vtkPeaksFilter::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgressText(message.c_str());
  this->UpdateProgress(progress);
}

/**
 * Extract the names of the peaks workspaces.
 * @returns A list of peaks workspace names.
 */
std::vector<std::string> vtkPeaksFilter::extractPeakWorkspaceNames()
{
  // Split the string in to bits 
  size_t pos = 0;
  std::string peakNames = m_peaksWorkspaceNames;
  std::vector<std::string> peaksWorkspaceNamesList;
  std::string token;
  while ((pos = peakNames.find(m_delimiter)) != std::string::npos) {
      token = peakNames.substr(0, pos);
      peaksWorkspaceNamesList.push_back(token);
      peakNames.erase(0, pos + m_delimiter.length());
  }

  // If there was only one element in there then push it
  if (peaksWorkspaceNamesList.empty()) {
    peaksWorkspaceNamesList.push_back(peakNames);
  }

  return peaksWorkspaceNamesList;
}

/**
 * Set the delimiter for concatenated workspace names.
 * @param delimiter The workspace name delimiter
 */
void vtkPeaksFilter::SetDelimiter(std::string delimiter){
  m_delimiter = delimiter;
  this->Modified();
}


/**
  * Get a list of peaks workspace pointers
  * @returns A list of peaks workspace pointers.
  */
std::vector<Mantid::API::IPeaksWorkspace_sptr> vtkPeaksFilter::getPeaksWorkspaces(std::vector<std::string> peaksWorkspaceNames)
{
  std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksWorkspaces;

  for (std::vector<std::string>::iterator it = peaksWorkspaceNames.begin(); it != peaksWorkspaceNames.end(); ++it)
  {
    // Check if the peaks workspace exists
    if (!Mantid::API::AnalysisDataService::Instance().doesExist(*it))
    {
      continue;
    }
    peaksWorkspaces.push_back(Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IPeaksWorkspace>(*it));
  }

  return peaksWorkspaces;
}

/**
 * Gets the minimum value of the data associated with the 
 * workspace.
 * @returns The minimum value of the workspace data.
 */
double vtkPeaksFilter::GetMinValue()
{
  return m_minValue;
}

/**
 * Gets the maximum value of the data associated with the 
 * workspace.
 * @returns The maximum value of the workspace data.
 */
double vtkPeaksFilter::GetMaxValue(){

   return m_maxValue;
}

/**
 * Getst the insturment name
 * @returns The name of the instrument.
 */
const char* vtkPeaksFilter::GetInstrument() {
  return m_instrument.c_str();
}

