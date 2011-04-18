#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/RebinningXMLGenerator.h"
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MantidAPI/Algorithm.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <vtkFloatArray.h>
#include <vtkFieldData.h>
#include <boost/shared_ptr.hpp>
#include <vtkCharArray.h>

namespace Mantid
{
namespace API
{
  //Forward declaration.
  class Algorithm;
}

namespace VATES
{
MultiDimensionalDbPresenter::MultiDimensionalDbPresenter() : m_isExecuted(false)
{

}

void MultiDimensionalDbPresenter::execute(API::Algorithm& algorithm, const std::string wsId)
{ 
  using namespace Mantid::API;

  if(true == algorithm.isInitialized())
  {
    algorithm.execute();
    extractWorkspaceImplementation(wsId);
    m_isExecuted = true;
  }
  else
  {
    throw std::invalid_argument("The algorithm parameter passed to this reader was not initalized");
  }
}

void MultiDimensionalDbPresenter::extractWorkspaceImplementation(const std::string& wsId)
{
  using namespace Mantid::API;
  Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsId);
  IMDWorkspace_sptr inputWS = boost::dynamic_pointer_cast<IMDWorkspace>(result);
  this->m_workspace = inputWS;
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
  //Sanity check. Must run execution successfully first.
  verifyExecution();

  return m_workspace->getXDimension()->getDimensionId();
}

std::string MultiDimensionalDbPresenter::getYAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_workspace->getYDimension()->getDimensionId();
}

std::string MultiDimensionalDbPresenter::getZAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_workspace->getZDimension()->getDimensionId();
}

vtkDataSet* MultiDimensionalDbPresenter::getMesh(RebinningXMLGenerator& serializer) const
{
  using namespace Mantid::MDDataObjects;

  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  //Create the mesh
  TimeStepToTimeStep timeStepMapper;
  vtkStructuredGridFactory<TimeStepToTimeStep> factory =  vtkStructuredGridFactory<TimeStepToTimeStep>::constructAsMeshOnly(m_workspace, timeStepMapper);
  vtkDataSet* visualDataSet = factory.createMeshOnly();

  vtkFieldData* outputFD = vtkFieldData::New();

  //Serialize metadata
  serializer.setWorkspaceName(m_workspace->getName());
  serializer.setWorkspaceLocation(m_workspace->getWSLocation());
  serializer.setGeometryXML(m_workspace->getGeometryXML());
  std::string xmlString = serializer.createXMLString();

  //Add metadata to dataset.
  MetadataToFieldData convert;
  convert(outputFD, xmlString, XMLDefinitions::metaDataId().c_str());
  visualDataSet->SetFieldData(outputFD);
  outputFD->Delete();
  return visualDataSet;
}

VecExtents MultiDimensionalDbPresenter::getExtents() const
{
  VecExtents extents;
  extents.push_back(0);
  extents.push_back(static_cast<int>( m_workspace->getXDimension()->getNBins() ));
  extents.push_back(0);
  extents.push_back(static_cast<int>( m_workspace->getYDimension()->getNBins() ));
  extents.push_back(0);
  extents.push_back(static_cast<int>( m_workspace->getZDimension()->getNBins() ));
  return extents;
}

size_t MultiDimensionalDbPresenter::getNumberOfTimesteps() const
{
  verifyExecution();
  return m_workspace->getTDimension()->getNBins();
}

std::vector<int> MultiDimensionalDbPresenter::getCycles() const
{
  verifyExecution();
  std::vector<int> cycles(m_workspace->getTDimension()->getNBins());
  for(unsigned int i=0; i < cycles.size(); i++)
    {
      cycles[i] = i;
    }
  return cycles;
}

std::vector<double> MultiDimensionalDbPresenter::getTimesteps() const
{
  using namespace Mantid::Geometry;
  verifyExecution();
  boost::shared_ptr<const IMDDimension> tDimension = m_workspace->getTDimension();
  const double increment = (tDimension->getMaximum() - tDimension->getMinimum())/tDimension->getNBins();
  std::vector<double> times(tDimension->getNBins());
  for(unsigned int i=0; i < tDimension->getNBins(); i++)
  {
    times[i] = tDimension->getMinimum() + (i*increment);
  }
  return times;
}

vtkDataArray* MultiDimensionalDbPresenter::getScalarDataFromTimeBin(int timeBin, const char* scalarName) const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();
  if(timeBin >= getNumberOfTimesteps())
  {
    throw std::range_error("A timestep larger than the range of available timesteps has been requested.");
  }

  TimeStepToTimeStep timeStepMapper;
  vtkStructuredGridFactory<TimeStepToTimeStep> scalarFactory(m_workspace, std::string(scalarName), timeBin, timeStepMapper);
  return scalarFactory.createScalarArray();
}

vtkDataArray* MultiDimensionalDbPresenter::getScalarDataFromTime(double time, const char* scalarName) const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();

  double tMax = m_workspace->getTDimension()->getMaximum();
  double tMin = m_workspace->getTDimension()->getMinimum();
  size_t nbins = m_workspace->getTDimension()->getNBins();

  TimeToTimeStep timeStepMapper(tMin, tMax, nbins);
  vtkStructuredGridFactory<TimeToTimeStep> scalarFactory(m_workspace, std::string(scalarName), time, timeStepMapper);
  return scalarFactory.createScalarArray();
}



MultiDimensionalDbPresenter::~MultiDimensionalDbPresenter()
{
}

}
}

