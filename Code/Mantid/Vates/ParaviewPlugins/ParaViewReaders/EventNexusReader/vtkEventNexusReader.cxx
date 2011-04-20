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
#include "vtkPlane.h"

#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

#include "MantidNexus/LoadEventNexus.h"
#include <boost/format.hpp>
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidMDEvents/OneStepMDEW.h"

#include "MantidMDAlgorithms/PlaneImplicitFunction.h"

vtkCxxRevisionMacro(vtkEventNexusReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkEventNexusReader);

using namespace Mantid::VATES;

vtkEventNexusReader::vtkEventNexusReader() : 
  m_presenter(), 
  m_isSetup(false), 
  m_mdEventWsId("eventWsId"), 
  m_histogrammedWsId("histogramWsId"),
  m_clipFunction(NULL)
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  //On first-pass rebinning is necessary.
  m_actionManager.ask(RecalculateAll);
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
    m_actionManager.ask(RecalculateAll);
  }
}

void vtkEventNexusReader::SetYBins(int nbins)
{
  if(nbins != m_nYBins)
  {
    m_nYBins = nbins;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

void vtkEventNexusReader::SetZBins(int nbins)
{
  if(nbins != m_nZBins)
  {
    m_nZBins = nbins;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

void vtkEventNexusReader::SetMaxThreshold(double maxThreshold)
{
  if(maxThreshold != m_maxThreshold)
  {
    m_maxThreshold = maxThreshold;
    this->Modified();
    m_actionManager.ask(RecalculateVisualDataSetOnly);
  }
}

void vtkEventNexusReader::SetMinThreshold(double minThreshold)
{
  if(minThreshold != m_minThreshold)
  {
    m_minThreshold = minThreshold;
    this->Modified();
    m_actionManager.ask(RecalculateVisualDataSetOnly);
  }
}

void vtkEventNexusReader::SetApplyClip(bool applyClip)
{
  if(m_applyClip != applyClip)
  {
    m_applyClip = applyClip;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

void vtkEventNexusReader::SetWidth(double width)
{
  if(m_width.getValue() != width)
  {
    m_width = width;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

void vtkEventNexusReader::SetClipFunction(vtkImplicitFunction* func)
{
  if(m_clipFunction != func)
  {
    m_clipFunction = func;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}


int vtkEventNexusReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
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

  //When RecalculateAll wins-out, configure and run the rebinning algorithm.
  if(RecalculateAll == m_actionManager.action())
  {
    Mantid::API::AnalysisDataService::Instance().remove(m_histogrammedWsId);

    Mantid::MDEvents::BinToMDHistoWorkspace hist_alg;
    hist_alg.initialize();
    hist_alg.setPropertyValue("InputWorkspace", m_mdEventWsId);
    hist_alg.setPropertyValue("DimX", boost::str(boost::format("Qx, -2.0, 2.0,  %d") % m_nXBins)); 
    hist_alg.setPropertyValue("DimY", boost::str(boost::format("Qy, -2.0, 2.0,  %d") % m_nYBins)); 
    hist_alg.setPropertyValue("DimZ", boost::str(boost::format("Qz, -2.0, 2.0,  %d") % m_nZBins)); 
    hist_alg.setPropertyValue("DimT", "NONE,0.0,10.0, 1");
    hist_alg.setPropertyValue("OutputWorkspace", m_histogrammedWsId);

    if(true == m_applyClip)
    {
      vtkPlane* plane = dynamic_cast<vtkPlane*>(this->m_clipFunction);
      if(NULL != plane)
      {
        //user has requested the use of implicit functions as part of rebinning. only planes understood for time being.
        using namespace Mantid::MDAlgorithms;
        double* pNormal = plane->GetNormal();
        double* pOrigin = plane->GetOrigin();
        NormalParameter normal(pNormal[0], pNormal[1], pNormal[2]);
        OriginParameter origin(pOrigin[0], pOrigin[1], pOrigin[2]);
        PlaneImplicitFunction func(normal, origin, m_width);
        hist_alg.setPropertyValue("ImplicitFunctionXML", func.toXMLString());
      }
    }

    FilterUpdateProgressAction<vtkEventNexusReader> updatehandler(this);
    // Run the algorithm and cache the output.
    m_presenter.execute(hist_alg, m_histogrammedWsId, updatehandler);
  }

  // This object determines how the visualization is made from a given imdworkspace.
  vtkThresholdingUnstructuredGridFactory<TimeToTimeStep> vtkGridFactory("signal", time, m_minThreshold, m_maxThreshold);

  RebinningXMLGenerator serializer(LocationNotRequired); //Object handles serialization of meta data.
  vtkUnstructuredGrid* structuredMesh = vtkUnstructuredGrid::SafeDownCast(m_presenter.getMesh(serializer, vtkGridFactory));

  output->ShallowCopy(structuredMesh);

  // Reset the action manager fresh for next cycle.
  m_actionManager.reset();
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

unsigned long vtkEventNexusReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->m_clipFunction != NULL)
  {

    time = this->m_clipFunction->GetMTime();
    if(time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}

void vtkEventNexusReader::UpdateAlgorithmProgress(double progress)
{
  this->SetProgressText("Executing Mantid MDEvent Rebinning Algorithm...");
  this->UpdateProgress(progress);
}