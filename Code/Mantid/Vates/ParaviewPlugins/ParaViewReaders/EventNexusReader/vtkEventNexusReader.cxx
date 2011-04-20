#include "vtkEventNexusReader.h"
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
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidNexus/LoadEventNexus.h"
#include <boost/format.hpp>
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidMDEvents/OneStepMDEW.h"

vtkCxxRevisionMacro(vtkEventNexusReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkEventNexusReader);

vtkEventNexusReader::vtkEventNexusReader() : m_presenter(), m_isSetup(false), m_mdEventWsId("eventWsId"), m_histogrammedWsId("histogramWsId")
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkEventNexusReader::~vtkEventNexusReader()
{
  this->SetFileName(0);
}

void vtkEventNexusReader::SetXBins(int nbins)
{
  if(nbins != m_nXBins)
  {
    m_nXBins = nbins;
    this->Modified();
  }
}

void vtkEventNexusReader::SetYBins(int nbins)
{
  if(nbins != m_nYBins)
  {
    m_nYBins = nbins;
    this->Modified();
  }
}

void vtkEventNexusReader::SetZBins(int nbins)
{
  if(nbins != m_nZBins)
  {
    m_nZBins = nbins;
    this->Modified();
  }
}

void vtkEventNexusReader::SetMaxThreshold(double maxThreshold)
{
  if(maxThreshold != m_maxThreshold)
  {
    m_maxThreshold = maxThreshold;
    this->Modified();
  }
}

void vtkEventNexusReader::SetMinThreshold(double minThreshold)
{
  if(minThreshold != m_minThreshold)
  {
    m_minThreshold = minThreshold;
    this->Modified();
  }
}

int vtkEventNexusReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  using namespace Mantid::VATES;
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int time;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
  {
    // usually only one actual step requested
    time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
  } 

  Mantid::API::AnalysisDataService::Instance().remove(m_histogrammedWsId);

  Mantid::MDEvents::BinToMDHistoWorkspace hist_alg;
  hist_alg.initialize();
  hist_alg.setPropertyValue("InputWorkspace", m_mdEventWsId);
  hist_alg.setPropertyValue("DimX", boost::str(boost::format("Qx, -2.0, 2.0,  %d") % m_nXBins)); 
  hist_alg.setPropertyValue("DimY", boost::str(boost::format("Qy, -2.0, 2.0,  %d") % m_nYBins)); 
  hist_alg.setPropertyValue("DimZ", boost::str(boost::format("Qz, -2.0, 2.0,  %d") % m_nZBins)); 
  hist_alg.setPropertyValue("DimT", "NONE,0.0,10.0, 1");  
  hist_alg.setPropertyValue("OutputWorkspace", m_histogrammedWsId);
  m_presenter.execute(hist_alg, m_histogrammedWsId);

  vtkThresholdingUnstructuredGridFactory<TimeToTimeStep> vtkGridFactory("signal", time, m_minThreshold, m_maxThreshold);

  RebinningXMLGenerator serializer(LocationNotRequired); //Object handles serialization of meta data.
  vtkUnstructuredGrid* structuredMesh = vtkUnstructuredGrid::SafeDownCast(m_presenter.getMesh(serializer, vtkGridFactory));

  output->ShallowCopy(structuredMesh);

  return 1;
}

int vtkEventNexusReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
 
  //Ensure that the Event Workspace is only generated once
  if(!m_isSetup) 
  {
    AnalysisDataService::Instance().remove("mdEventWsId");

    Mantid::MDEvents::OneStepMDEW alg;
    alg.initialize();
    alg.setPropertyValue("Filename", this->FileName);
    alg.setPropertyValue("OutputWorkspace", m_mdEventWsId);
    alg.execute();
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

void vtkEventNexusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkEventNexusReader::CanReadFile(const char* vtkNotUsed(fname))
{
  return 1; //TODO: Apply checks here.
}
