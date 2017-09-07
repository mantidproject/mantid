#include "vtkPeaksFilter.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"

#include <boost/scoped_ptr.hpp>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFieldData.h>

vtkStandardNewMacro(vtkPeaksFilter)

using namespace Mantid::VATES;

vtkPeaksFilter::vtkPeaksFilter()
    : m_radiusNoShape(0.5), m_coordinateSystem(0),
      m_radiusType(Mantid::Geometry::PeakShape::Radius),
      m_metadataJsonManager(new MetadataJsonManager()),
      m_vatesConfigurations(new VatesConfigurations()) {
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

  // If the field data does not contain the metadata, then don't do anything.
  try
  {
    vtkFieldData* fieldData = inputDataSet->GetFieldData();
  
    // Extract information for meta data in Json format.
    FieldDataToMetadata fieldDataToMetadata;

    std::string jsonString = fieldDataToMetadata(fieldData, m_vatesConfigurations->getMetadataIdJson());
    m_metadataJsonManager->readInSerializedJson(jsonString);

    m_instrument = m_metadataJsonManager->getInstrument();
    m_coordinateSystem = m_metadataJsonManager->getSpecialCoordinates();
  }
  catch (...)
  {
  }

  if (m_peaksWorkspaces.empty()) {
    return 0;
  }
  FilterUpdateProgressAction<vtkPeaksFilter> drawingProgressUpdate(this, "Drawing...");

  vtkDataSetToPeaksFilteredDataSet peaksFilter(inputDataSet, outputDataSet);
  peaksFilter.initialize(m_peaksWorkspaces, m_radiusNoShape, m_radiusType,
                         m_coordinateSystem);
  peaksFilter.execute(drawingProgressUpdate);
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
    m_instrument = m_metadataJsonManager->getInstrument();
    m_coordinateSystem = m_metadataJsonManager->getSpecialCoordinates();
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
 * @param delimiter The workspace name delimiter
*/
void vtkPeaksFilter::SetPeaksWorkspace(const std::string &peaksWorkspaceName,
                                       const std::string &delimiter) {
  auto tokenizedNames =
      Mantid::Kernel::StringTokenizer(peaksWorkspaceName, delimiter);
  m_peaksWorkspaces = getPeaksWorkspaces(tokenizedNames);
  this->Modified();
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
void vtkPeaksFilter::SetRadiusType(int type) {
  m_radiusType = static_cast<Mantid::Geometry::PeakShape::RadiusType>(type);
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
  * Get a list of peaks workspace pointers
  * @returns A list of peaks workspace pointers.
  */
std::vector<Mantid::API::IPeaksWorkspace_sptr>
vtkPeaksFilter::getPeaksWorkspaces(
    const Mantid::Kernel::StringTokenizer &workspaceNames) {

  Mantid::API::AnalysisDataServiceImpl &ADS =
      Mantid::API::AnalysisDataService::Instance();

  std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksWorkspaces;
  for (const auto &name : workspaceNames) {
    // Check if the peaks workspace exists
    if (ADS.doesExist(name)) {
      peaksWorkspaces.push_back(
          ADS.retrieveWS<Mantid::API::IPeaksWorkspace>(name));
    }
  }
  return peaksWorkspaces;
}

/**
 * Getst the insturment name
 * @returns The name of the instrument.
 */
const char* vtkPeaksFilter::GetInstrument() {
  return m_instrument.c_str();
}

