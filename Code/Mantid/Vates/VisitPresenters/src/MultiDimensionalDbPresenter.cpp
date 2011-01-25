#include <MantidVisitPresenters/MultiDimensionalDbPresenter.h>
#include "MantidVisitPresenters/RebinningCutterXMLDefinitions.h"
#include "MantidVisitPresenters/RebinningXMLGenerator.h"
#include "MantidVisitPresenters/GenerateStructuredGrid.h"
#include <MantidGeometry/MDGeometry/MDGeometry.h>
#include <MDDataObjects/IMD_FileFormat.h>
#include <MDDataObjects/MD_FileFormatFactory.h>
#include <MDDataObjects/MDWorkspace.h>
#include <MantidMDAlgorithms/CenterpieceRebinning.h>
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <vtkFloatArray.h>
#include <vtkFieldData.h>
#include <boost/shared_ptr.hpp>
#include <vtkCharArray.h>

namespace Mantid
{
namespace VATES
{
MultiDimensionalDbPresenter::MultiDimensionalDbPresenter() : m_isExecuted(false)
{

}

void MultiDimensionalDbPresenter::execute(const std::string& fileName)
{
  using namespace Mantid::MDAlgorithms;
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::API;

  Load_MDWorkspace wsLoaderAlg;
  wsLoaderAlg.initialize();
  std::string wsId = "InputMDWs";
  wsLoaderAlg.setPropertyValue("inFilename", fileName);
  wsLoaderAlg.setPropertyValue("MDWorkspace",wsId);
  wsLoaderAlg.execute();
  Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsId);
  MDWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MDWorkspace>(result);
  this->m_MDWorkspace = inputWS;

  m_isExecuted = true;
}

void MultiDimensionalDbPresenter::verifyExecution() const
{
  if(!this->m_isExecuted)
  {
    throw std::runtime_error("Cannot get mesh or get variables until rebinning has occured via ::execute()");
  }
}

std::string MultiDimensionalDbPresenter::getXAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_MDWorkspace->getGeometry()->getXDimension()->getDimensionId();
}

std::string MultiDimensionalDbPresenter::getYAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_MDWorkspace->getGeometry()->getYDimension()->getDimensionId();
}

std::string MultiDimensionalDbPresenter::getZAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_MDWorkspace->getGeometry()->getZDimension()->getDimensionId();
}

vtkDataSet* MultiDimensionalDbPresenter::getMesh() const
{
  using namespace Mantid::MDDataObjects;

  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  //Create the mesh
  GenerateStructuredGrid meshGenerator(m_MDWorkspace);
  vtkDataSet* visualDataSet = meshGenerator.create();

  vtkFieldData* outputFD = vtkFieldData::New();

  //Serialize metadata
  RebinningXMLGenerator serializer;
  serializer.setWorkspaceName(m_MDWorkspace->getName());
  serializer.setWorkspaceLocation(m_MDWorkspace->getWSLocation());
  serializer.setGeometryXML(m_MDWorkspace->get_const_MDGeometry().toXMLString());
  std::string xmlString = serializer.createXMLString();

  //Add metadata to dataset.
  MultiDimensionalDbPresenter::metaDataToFieldData(outputFD, xmlString, XMLDefinitions::metaDataId.c_str());
  visualDataSet->SetFieldData(outputFD);
  return visualDataSet;
}

void MultiDimensionalDbPresenter::metaDataToFieldData(vtkFieldData* fieldData, std::string metaData,
    const char* id) const
{
  //clean out existing.
  vtkDataArray* arry = fieldData->GetArray(id);
  if(NULL != arry)
  {
    fieldData->RemoveArray(id);
  }
  //create new.
  vtkCharArray* newArry = vtkCharArray::New();
  newArry->Allocate(metaData.size());
  newArry->SetName(id);
  fieldData->AddArray(newArry);

  for(unsigned int i = 0 ; i < metaData.size(); i++)
  {
    newArry->InsertNextValue(metaData.at(i));
  }
}

int MultiDimensionalDbPresenter::getNumberOfTimesteps() const
{
  verifyExecution();
  return m_MDWorkspace->gettDimension()->getNBins();
}

vtkDataArray* MultiDimensionalDbPresenter::getScalarData(int timeBin, const char* scalarName) const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();
  if(timeBin >= getNumberOfTimesteps())
  {
    throw std::range_error("A timestep larger than the range of available timesteps has been requested.");
  }

  boost::shared_ptr<const MDImage> spImage(m_MDWorkspace->get_spMDImage());
  const int sizeX = m_MDWorkspace->getXDimension()->getNBins();
  const int sizeY = m_MDWorkspace->getYDimension()->getNBins();
  const int sizeZ = m_MDWorkspace->getZDimension()->getNBins();

  vtkFloatArray* scalars = vtkFloatArray::New();
  scalars->SetName(scalarName);
  scalars->Allocate(sizeX * sizeY * sizeZ);

  //Loop through dimensions
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        MD_image_point point = spImage->getPoint(i, j, k, timeBin);
        scalars->InsertNextValue(point.s);
      }
    }
  }

  return scalars;
}

MultiDimensionalDbPresenter::~MultiDimensionalDbPresenter()
{
}

}
}

