#include "vtkPeaksReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPlane.h"

#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/vtkThresholdingHexahedronFactory.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "MantidVatesAPI/vtkThresholdingLineFactory.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"

#include <boost/format.hpp>
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"

vtkCxxRevisionMacro(vtkPeaksReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkPeaksReader);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::AnalysisDataService;

vtkPeaksReader::vtkPeaksReader() :
  m_presenter(), 
  m_isSetup(false), 
  m_mdEventWsId("eventWsId"),
  m_histogrammedWsId("histogramWsId")
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  //On first-pass rebinning is necessary.
  m_actionManager.ask(RecalculateAll);
}

vtkPeaksReader::~vtkPeaksReader()
{
  this->SetFileName(0);
}

/**
  Sets width for plane.
  @param width: width for plane.
*/
void vtkPeaksReader::SetWidth(double width)
{
  if(m_width != width)
  {
    m_width = width;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

  
/**
  Sets applied geometry xml. Provided by object panel.
  @xml. Dimension xml.
*/
void vtkPeaksReader::SetAppliedGeometryXML(std::string appliedGeometryXML)
{
  UNUSED_ARG(appliedGeometryXML);
  if(m_isSetup)
  {
    // DO some set up here???
  }
}

/**
  Gets the geometry xml from the workspace. Allows object panels to configure themeselves.
  @return geometry xml const * char reference.
*/
const char* vtkPeaksReader::GetInputGeometryXML()
{
  return this->m_geometryXmlBuilder.create().c_str();
}


/**
   Actually perform the rebinning. Configure the rebinning algorithm and pass to presenter.
 */
void vtkPeaksReader::doRebinning()
{
  // ?????

  FilterUpdateProgressAction<vtkPeaksReader> updatehandler(this);

  // Run the algorithm and cache the output.
  //m_presenter.execute(hist_alg, m_histogrammedWsId, updatehandler);
}


int vtkPeaksReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int time = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
  {
    // usually only one actual step requested
    time = static_cast<int>(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0]);
  } 

  //When RecalculateAll wins-out, configure and run the rebinning algorithm.
  if(RecalculateAll == m_actionManager.action())
  {
    doRebinning();
  }

  // Chain of resposibility setup for visualisation. Encapsulates decision making on how workspace will be rendered.
  std::string scalarName = "signal";

  // Instantiate the factory that makes the peak markers
  vtkPeakMarkerFactory * p_peakFactory = new vtkPeakMarkerFactory(scalarName);

  Workspace_sptr result=AnalysisDataService::Instance().retrieve("LoadedPeaksWS");
  Mantid::API::IPeaksWorkspace_sptr peakWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(result);

  p_peakFactory->initialize(peakWS);
  vtkDataSet * structuredMesh = p_peakFactory->create();

  output->ShallowCopy(structuredMesh);


//  RebinningKnowledgeSerializer serializer(LocationNotRequired); //Object handles serialization of meta data.
//  vtkUnstructuredGrid * structuredMesh = vtkUnstructuredGrid::SafeDownCast(m_presenter.getMesh(serializer, *p_peakFactory));
//  output->ShallowCopy(structuredMesh);



//  vtkThresholdingLineFactory vtkGridFactory(scalarName, m_minThreshold, m_maxThreshold);
//  vtkThresholdingQuadFactory* p_2dSuccessorFactory = new vtkThresholdingQuadFactory(scalarName, m_minThreshold, m_maxThreshold);
  //  vtkThresholdingHexahedronFactory* p_3dSuccessorFactory = new vtkThresholdingHexahedronFactory(scalarName, m_minThreshold, m_maxThreshold);
//  vtkGridFactory.SetSuccessor(p_2dSuccessorFactory);
//  p_2dSuccessorFactory->SetSuccessor(p_3dSuccessorFactory);
//
//  RebinningKnowledgeSerializer serializer(LocationNotRequired); //Object handles serialization of meta data.
//  vtkUnstructuredGrid* structuredMesh = vtkUnstructuredGrid::SafeDownCast(m_presenter.getMesh(serializer, vtkGridFactory));
//
//  output->ShallowCopy(structuredMesh);

  // Reset the action manager fresh for next cycle.
  m_actionManager.reset();
  return 1;
}



int vtkPeaksReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  using namespace Mantid::API;
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
 
  //Ensure that the Event Workspace is only generated once
  if(!m_isSetup) 
  {
    // This actually loads the peaks file
    IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("LoadPeaksFile", -1);
    alg->initialize();
    alg->setPropertyValue("Filename", this->FileName);
    alg->setPropertyValue("OutputWorkspace", "LoadedPeaksWS");
    alg->execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve("LoadedPeaksWS");
    Mantid::API::IPeaksWorkspace_sptr peakWS = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(result);
    m_isSetup = true;
  }

  std::vector<double> timeStepValues(1); //TODO set time-step information.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0], static_cast<int>(timeStepValues.size()));
  double timeRange[2];
  timeRange[0] = timeStepValues.front();
  timeRange[1] = timeStepValues.back();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}

void vtkPeaksReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkPeaksReader::CanReadFile(const char* vtkNotUsed(fname))
{
  return 1; //TODO: Apply checks here.
}

unsigned long vtkPeaksReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  return mTime;
}

/**
  Update/Set the progress.
  @parameter progress : progress increment.
*/
void vtkPeaksReader::UpdateAlgorithmProgress(double progress)
{
  this->SetProgressText("Executing Mantid MDEvent Rebinning Algorithm...");
  this->UpdateProgress(progress);
}
