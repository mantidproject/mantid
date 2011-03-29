#include "vtkRebinningCutter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkPVClipDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImplicitFunction.h"
#include "vtkPointData.h"
#include "vtkBox.h"

#include "MantidKernel/Exception.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/ImageProxy.h"
#include "MantidVatesAPI/vtkProxyFactory.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include <boost/functional/hash.hpp>
#include <sstream>
#include "ParaViewProgressAction.h"




/** Plugin for ParaView. Performs simultaneous rebinning and slicing of Mantid data.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

vtkCxxRevisionMacro(vtkRebinningCutter, "$Revision: 1.0 $")
;
vtkStandardNewMacro(vtkRebinningCutter)
;

using namespace Mantid::VATES;

vtkRebinningCutter::vtkRebinningCutter() :
  m_presenter(),
  m_clipFunction(NULL),
  m_cachedVTKDataSet(NULL),
  m_isSetup(false),
  m_timestep(0),
  m_thresholdMax(10000),
  m_thresholdMin(0),
  m_actionRequester()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkRebinningCutter::~vtkRebinningCutter()
{
}

std::string vtkRebinningCutter::createRedrawHash() const
{
  size_t seed = 1;
  using namespace boost;
  hash_combine(seed, m_thresholdMax);
  hash_combine(seed, m_thresholdMin);
  //TODO add other properties that should force redraw only when changed.
  std::stringstream sstream;
  sstream << seed;
  return sstream.str();
}

void vtkRebinningCutter::determineAnyCommonExecutionActions(const int timestep, BoxFunction_sptr box)
{
  //Handles some commong iteration actions that can only be determined at execution time.
  if (NULL == m_cachedVTKDataSet)
  {
    m_actionRequester.ask(RecalculateAll);
  }
  if ((timestep != m_timestep))
  {
    m_actionRequester.ask(RecalculateVisualDataSetOnly);
  }
  if (m_cachedRedrawArguments != createRedrawHash())
  {
    m_actionRequester.ask(RecalculateVisualDataSetOnly);
  }
  if(m_box.get() != NULL && *m_box != *box)
  {
     m_actionRequester.ask(RecalculateAll); //The clip function must have changed.
  }
}

int vtkRebinningCutter::RequestData(vtkInformation *request, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  using namespace Mantid::Geometry;
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;
  using Mantid::VATES::Dimension_sptr;
  using Mantid::VATES::DimensionVec;

  //Setup is not complete until metadata has been correctly provided.
  if(true == m_isSetup)
  {
    vtkInformation * inputInf = inputVector[0]->GetInformationObject(0);
    vtkDataSet * inputDataset = vtkDataSet::SafeDownCast(inputInf->Get(vtkDataObject::DATA_OBJECT()));

    DimensionVec dimensionsVec(4);
    dimensionsVec[0] = m_appliedXDimension;
    dimensionsVec[1] = m_appliedYDimension;
    dimensionsVec[2] = m_appliedZDimension;
    dimensionsVec[3] = m_appliedTDimension;

    //Create the composite holder.
    CompositeImplicitFunction* compFunction = new CompositeImplicitFunction;
    BoxFunction_sptr box = constructBox(inputDataset); // Save the implicit function so that we may later determine if the extents have changed.
    compFunction->addFunction(box);
    
    // Construct reduction knowledge.
    m_presenter.constructReductionKnowledge(
      dimensionsVec,
      m_appliedXDimension,
      m_appliedYDimension,
      m_appliedZDimension,
      m_appliedTDimension,
      compFunction,
      inputDataset);

    ParaViewProgressAction updatehandler(this);

    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(
      vtkDataObject::DATA_OBJECT()));

    //Acutally perform rebinning or specified action.
    int timestep = 0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
      // usually only one actual step requested
      timestep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
    }
    //TODO: warn if time falls outside of time range vtkWarnMacro ...
    determineAnyCommonExecutionActions(timestep, box);

    vtkUnstructuredGrid* outData;
    RebinningIterationAction action = m_actionRequester.action();
    MDWorkspace_sptr spRebinnedWs = m_presenter.applyRebinningAction(action, updatehandler);
    if (UseCache == action)
    {
      //Use existing vtkDataSet
      vtkDataSetFactory_sptr spvtkDataSetFactory(new vtkProxyFactory(m_cachedVTKDataSet));
      outData = dynamic_cast<vtkUnstructuredGrid*> (m_presenter.createVisualDataSet(spvtkDataSetFactory));
      outData->Register(NULL);
    }
    else
    {
      //Build a vtkDataSet
      vtkDataSetFactory_sptr spvtkDataSetFactory = createDataSetFactory(spRebinnedWs);

      outData = dynamic_cast<vtkUnstructuredGrid*> (m_presenter.createVisualDataSet(spvtkDataSetFactory));
      m_cachedVTKDataSet = outData;
    }
    m_timestep = timestep; //Not settable directly via a setter.
    m_box = box;
    m_cachedRedrawArguments = createRedrawHash();
    m_actionRequester.reset();

  /*  vtkBox* vbox = dynamic_cast<vtkBox*>(this->m_clipFunction);
  vbox->SetBounds(0, 2, 0, 2, 0, 20);*/

    output->ShallowCopy(outData);
  }
  return 1;
}

void vtkRebinningCutter::UpdateAlgorithmProgress(double progress)
{
  this->SetProgressText("Executing Mantid Rebinning Algorithm...");
  this->UpdateProgress(progress);
}

int vtkRebinningCutter::RequestInformation(vtkInformation *request, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  enum Status{Bad=0, Good=1};
  Status status=Good;
  if (!m_isSetup)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;
    using Mantid::VATES::Dimension_sptr;
    using Mantid::VATES::DimensionVec;

    vtkInformation * inputInf = inputVector[0]->GetInformationObject(0);
    vtkDataSet * inputDataset = vtkDataSet::SafeDownCast(inputInf->Get(vtkDataObject::DATA_OBJECT()));

    bool bGoodInputProvided = canProcessInput(inputDataset);
    if(true == bGoodInputProvided)
    { 
      DimensionVec dimensionsVec(4);
      dimensionsVec[0] = m_presenter.getXDimensionFromDS(inputDataset);
      dimensionsVec[1] = m_presenter.getYDimensionFromDS(inputDataset);
      dimensionsVec[2] = m_presenter.getZDimensionFromDS(inputDataset);
      dimensionsVec[3] = m_presenter.getTDimensionFromDS(inputDataset);

      m_appliedXDimension = dimensionsVec[0];
      m_appliedYDimension = dimensionsVec[1];
      m_appliedZDimension = dimensionsVec[2];
      m_appliedTDimension = dimensionsVec[3];

      // Construct reduction knowledge.
      m_presenter.constructReductionKnowledge(dimensionsVec, dimensionsVec[0], dimensionsVec[1],
        dimensionsVec[2], dimensionsVec[3], inputDataset);

      m_isSetup = true;
    }
    else
    {
      vtkErrorMacro("Rebinning operations require Rebinning Metadata. Have you provided a rebinning source?");
      m_isSetup = false;
      status = Bad;
    }
  }
  setTimeRange(outputVector);
  return status;
  
}

int vtkRebinningCutter::RequestUpdateExtent(vtkInformation* info, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  return 1;
}
;

int vtkRebinningCutter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkRebinningCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkRebinningCutter::SetClipFunction(vtkImplicitFunction * func)
{
  if (func != m_clipFunction)
  {
    this->Modified();
    this->m_clipFunction = func;
  }
}

void vtkRebinningCutter::SetMaxThreshold(double maxThreshold)
{
  if (maxThreshold != m_thresholdMax)
  {
    this->Modified();
    this->m_thresholdMax = maxThreshold;
  }
}

void vtkRebinningCutter::SetMinThreshold(double minThreshold)
{
  if (minThreshold != m_thresholdMin)
  {
    this->Modified();
    this->m_thresholdMin = minThreshold;
  }
}

void vtkRebinningCutter::formulateRequestUsingNBins(Mantid::VATES::Dimension_sptr newDim)
{
  using Mantid::VATES::Dimension_const_sptr;
  try
  {
     //Requests that the dimension is found in the workspace.
     Dimension_const_sptr wsDim = m_presenter.getDimensionFromWorkspace(newDim->getDimensionId());
     //
     if(newDim->getNBins() != wsDim->getNBins())
     {
       //The number of bins has changed. Rebinning cannot be avoided.
       m_actionRequester.ask(RecalculateAll);
     }
  }
  catch(Mantid::Kernel::Exception::NotFoundError&)
  {
    //This happens if the workspace is not available in the analysis data service. Hence the rebinning algorithm has not yet been run.
    m_actionRequester.ask(RecalculateAll);
  }
}

void vtkRebinningCutter::SetAppliedXDimensionXML(std::string xml)
{
  if (NULL != m_appliedXDimension.get())
  {
    if (m_appliedXDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      Mantid::VATES::Dimension_sptr temp = Mantid::VATES::createDimension(xml);
      m_actionRequester.ask(RecalculateVisualDataSetOnly);
      //The visualisation dataset will at least need to be recalculated.
      formulateRequestUsingNBins(temp);
      this->m_appliedXDimension = temp;
    }
  }
}

void vtkRebinningCutter::SetAppliedYDimensionXML(std::string xml)
{
  if (NULL != m_appliedYDimension.get())
  {
    if (m_appliedYDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      Mantid::VATES::Dimension_sptr temp = Mantid::VATES::createDimension(xml);
      //The visualisation dataset will at least need to be recalculated.
      m_actionRequester.ask(RecalculateVisualDataSetOnly);
      formulateRequestUsingNBins(temp);
      this->m_appliedYDimension = temp;
    }
  }
}

void vtkRebinningCutter::SetAppliedZDimensionXML(std::string xml)
{
  if (NULL != m_appliedZDimension.get())
  {
    if (m_appliedZDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      Mantid::VATES::Dimension_sptr temp = Mantid::VATES::createDimension(xml);
      //The visualisation dataset will at least need to be recalculated.
      m_actionRequester.ask(RecalculateVisualDataSetOnly); 
      formulateRequestUsingNBins(temp);
      this->m_appliedZDimension = temp;
    }
  }
}

void vtkRebinningCutter::SetAppliedtDimensionXML(std::string xml)
{
  if (NULL != m_appliedTDimension.get())
  {
    if (m_appliedTDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      Mantid::VATES::Dimension_sptr temp = Mantid::VATES::createDimension(xml);
      //The visualisation dataset will at least need to be recalculated.
      m_actionRequester.ask(RecalculateVisualDataSetOnly);
      formulateRequestUsingNBins(temp);
      this->m_appliedTDimension = temp;
    }
  }
}

const char* vtkRebinningCutter::GetInputGeometryXML()
{
  try
  {
    return this->m_presenter.getWorkspaceGeometry().c_str();
  }
  catch(std::runtime_error& err)
  {
    return "";
  }
}

unsigned long vtkRebinningCutter::GetMTime()
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

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensionX(vtkDataSet* in_ds) const
{
  return m_presenter.getXDimensionFromDS(in_ds);
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensionY(vtkDataSet* in_ds) const
{
  return m_presenter.getYDimensionFromDS(in_ds);
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensionZ(vtkDataSet* in_ds) const
{
  return m_presenter.getZDimensionFromDS(in_ds);
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensiont(vtkDataSet* in_ds) const
{
  return m_presenter.getTDimensionFromDS(in_ds);
}


BoxFunction_sptr vtkRebinningCutter::constructBox(vtkDataSet* inputDataset) const
{
  using namespace Mantid::MDAlgorithms;
  
  vtkBox* box = dynamic_cast<vtkBox*>(this->m_clipFunction);

  //To get the box bounds, we actually need to evaluate the box function. There is not this restriction on planes.
  vtkPVClipDataSet * cutter = vtkPVClipDataSet::New();
	cutter->SetInput(inputDataset);
	cutter->SetClipFunction(box);
  cutter->SetInsideOut(true);
	cutter->Update();
  vtkDataSet* cutterOutput = cutter->GetOutput();
  //Now we can get the bounds.
  double* bounds = cutterOutput->GetBounds();
  
  double originX = (bounds[1] + bounds[0]) / 2;
  double originY = (bounds[3] + bounds[2]) / 2;
  double originZ = (bounds[5] + bounds[4]) / 2;
  double width = sqrt(pow(bounds[1] - bounds[0], 2));
  double height = sqrt(pow(bounds[3] - bounds[2], 2));
  double depth = sqrt(pow(bounds[5] - bounds[4], 2));
  
  //Create domain parameters.
  OriginParameter originParam = OriginParameter(originX, originY, originZ);
  WidthParameter widthParam = WidthParameter(width);
  HeightParameter heightParam = HeightParameter(height);
  DepthParameter depthParam = DepthParameter(depth);

  //Create the box. This is specific to this type of presenter and this type of filter. Other rebinning filters may use planes etc.
  BoxImplicitFunction* boxFunction = new BoxImplicitFunction(widthParam, heightParam, depthParam,
      originParam);

  return BoxFunction_sptr(boxFunction);
}

vtkDataSetFactory_sptr vtkRebinningCutter::createDataSetFactory(
    Mantid::MDDataObjects::MDWorkspace_sptr spRebinnedWs) const
{
  if(m_actionRequester.action() == RecalculateAll)
  {
    return createQuickRenderDataSetFactory(spRebinnedWs);
  }
  else
  {
    return createQuickChangeDataSetFactory(spRebinnedWs);
  }
}

vtkDataSetFactory_sptr vtkRebinningCutter::createQuickChangeDataSetFactory(
    Mantid::MDDataObjects::MDWorkspace_sptr spRebinnedWs) const
{
  using Mantid::MDDataObjects::MDImage;

  //Get the time dimension
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> timeDimension = spRebinnedWs->getTDimension();

  //Create a mapper to transform real time into steps.
  TimeToTimeStep timeMapper(timeDimension->getMinimum(), timeDimension->getMaximum(),
      timeDimension->getNBins());

  GeometryProxy* geomProxy = GeometryProxy::New
      (spRebinnedWs->get_spMDImage()->getGeometry(),
          m_appliedXDimension,
          m_appliedYDimension,
          m_appliedZDimension,
          m_appliedTDimension);

 boost::shared_ptr<ImageProxy> spImageProcessor(ImageProxy::New(geomProxy, spRebinnedWs->get_spMDImage()));

  //Create a factory for generating a thresholding unstructured grid.
  vtkDataSetFactory* pvtkDataSetFactory = new vtkThresholdingUnstructuredGridFactory<ImageProxy,
      TimeToTimeStep> (spImageProcessor, XMLDefinitions::signalName(), m_timestep,
      timeMapper, m_thresholdMin, m_thresholdMax);

  //Return the generated factory.
  return vtkDataSetFactory_sptr(pvtkDataSetFactory);
}

vtkDataSetFactory_sptr vtkRebinningCutter::createQuickRenderDataSetFactory(
    Mantid::MDDataObjects::MDWorkspace_sptr spRebinnedWs) const
{
  using Mantid::MDDataObjects::MDImage;

  //Create a mapper to transform real time into steps.
  TimeToTimeStep timeMapper(m_appliedTDimension->getMinimum(), m_appliedTDimension->getMaximum(),
      m_appliedTDimension->getNBins());

  //Create a factory for generating a thresholding unstructured grid.
  vtkDataSetFactory* pvtkDataSetFactory = new vtkThresholdingUnstructuredGridFactory<MDImage,
      TimeToTimeStep> (spRebinnedWs->get_spMDImage(), XMLDefinitions::signalName(), m_timestep,
      timeMapper, m_thresholdMin, m_thresholdMax);

  //Return the generated factory.
  return vtkDataSetFactory_sptr(pvtkDataSetFactory);
}

void vtkRebinningCutter::setTimeRange(vtkInformationVector* outputVector)
{
  if(true == m_isSetup)
  {
    double min = m_appliedTDimension->getMinimum();
    double max = m_appliedTDimension->getMaximum();
    unsigned int nBins = m_appliedTDimension->getNBins();
    double increment = (max - min) / nBins;
    std::vector<double> timeStepValues(nBins);
    for (unsigned int i = 0; i < nBins; i++)
    {
      timeStepValues[i] = min + (i * increment);
    }
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0],
      static_cast<int> (timeStepValues.size()));
    double timeRange[2];
    timeRange[0] = timeStepValues.front();
    timeRange[1] = timeStepValues.back();

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
}

